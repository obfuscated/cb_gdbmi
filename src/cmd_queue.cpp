#include "cmd_queue.h"

#include <algorithm>
#include <logmanager.h>
#include <pipedprocess.h>

#include "cmd_result_parser.h"
#include "events.h"

namespace dbg_mi
{

Command::~Command()
{
    delete m_result;
}

void Action::MarkStarted()
{
    m_started = true;
}

void Action::SetCommandResult(int32_t cmd_id, ResultParser *parser)
{
    assert(m_command_results.find(cmd_id) == m_command_results.end()
           && "can't receiv result for the same command twice!");
    m_command_results[cmd_id] = parser;

    OnCommandResult(cmd_id);
}

ResultParser* Action::GetCommandResult(int32_t cmd_id)
{
    CommandResult::iterator it = m_command_results.find(cmd_id);
    return it != m_command_results.end() ? it->second : NULL;
}

int32_t Action::QueueCommand(wxString const &cmd_string)
{
    Command *cmd = new Command;
    cmd->SetString(cmd_string);
    cmd->SetActionID(GetID());

    cmd->SetID(m_last_cmd_id++);

    m_queue->AddCommand(cmd, false);
    return cmd->GetID();
}

void Action::Finish()
{
    m_queue->RemoveAction(this);
}

void Action::Log(wxString const &s)
{
    m_queue->Log(s);
}
void Action::DebugLog(wxString const &s)
{
    m_queue->DebugLog(s);
}


CommandQueue::CommandQueue() :
    m_last_cmd_id(0),
    m_last_action_id(1), // action_id == 0 is reserved for actionless commands
    m_normal_log(-1),
    m_debug_log(-1)
{
}

CommandQueue::~CommandQueue()
{
    for(Queue::iterator it = m_commands_to_execute.begin(); it != m_commands_to_execute.end(); ++it)
        delete *it;
    for(CommandMap::iterator it = m_active_commands.begin(); it != m_active_commands.end(); ++it)
        delete it->second;
    for(ActionMap::iterator it = m_active_actions.begin(); it != m_active_actions.end(); ++it)
        delete it->second;
}

int64_t CommandQueue::AddCommand(Command *command, bool generate_id)
{
    if(generate_id)
        command->SetID(m_last_cmd_id++);

    wxString full_cmd = wxString::Format(_T("%d (last: %d)"), command->GetID(), m_last_cmd_id) + command->GetString();
    DebugLog(_("adding commnad: ") + full_cmd);

    m_commands_to_execute.push_back(command);
    return command->GetFullID();
}
void CommandQueue::RunQueue(PipedProcess *process)
{
    Queue executed_cmds;
    while(!m_commands_to_execute.empty())
    {
        Command *command = m_commands_to_execute.front();
        m_commands_to_execute.pop_front();

        wxString full_cmd = wxString::Format(_T("%d%010d"), command->GetActionID(), command->GetID())
                            + command->GetString();

        DebugLog(_("sending commnd: ") + full_cmd);
        process->SendString(full_cmd);

        executed_cmds.push_back(command);
    }

    {
        wxMutexLocker locker(m_lock);
        for(Queue::iterator it = executed_cmds.begin(); it != executed_cmds.end(); ++it)
        {
            DebugLog(wxString::Format(wxT("Adding active command %010ld"), (*it)->GetFullID()));
            m_active_commands[(*it)->GetFullID()] = *it;
        }
    }
}
void CommandQueue::AccumulateOutput(wxString const &output)
{
    m_full_output += output;
    m_full_output += _T("\n");
    ParseOutput();
}

void CommandQueue::ParseOutput()
{
    int start = -1, end = -1;
    int last_end = -1, token_start = -1;
    ResultParser::Type result_type = ResultParser::Result;

    for(size_t ii = 0; ii < m_full_output.length(); ++ii)
    {
        wxChar ch = m_full_output[ii];

        switch(ch)
        {
        case _T('^'): // result record
            if(start == -1)
            {
                result_type = ResultParser::Result;
                start = ii;
            }
            break;
        case _T('*'):
            if(start == -1)
            {
                result_type = ResultParser::ExecAsyncOutput;
                start = ii;
            }
            break;
        case _T('+'):
            if(start == -1)
            {
                result_type = ResultParser::StatusAsyncOutput;
                start = ii;
            }
            break;
        case _T('='):
            if(start == -1)
            {
                result_type = ResultParser::NotifyAsyncOutput;
                start = ii + 1;
            }
            break;
        case _T('\n'):
        case _T('\r'):
            end = ii;
            break;
        }

        if(start == -1)
        {
            if(wxIsdigit(ch))
            {
                if(token_start == -1)
                    token_start = ii;
            }
            else
            {
                if(token_start >= 0)
                    token_start = -1;
            }
        }


        if(start >= 0 && end >= 0)
        {
            wxString token_str = m_full_output.substr(token_start, start - token_start);
            wxString line = m_full_output.substr(start + 1, end - start - 1);
            DebugLog(_("token:") + token_str + _(";parsed line >>: ") + line + _(":<<"));

            last_end = end;
            start = end = -1;
            token_start = -1;

            wxString cmd_token_str = token_str.substr(std::max(token_str.length() - 10, 0lu), 10);
            wxString action_token_str = token_str.substr(0, std::max(token_str.length() - 10, 0lu));

            long cmd_token, action_token;

            if(!cmd_token_str.ToLong(&cmd_token, 10))
            {
                DebugLog(_("cmd_token is not int:") + cmd_token_str + _T(";"));
            }
            else if(!action_token_str.ToLong(&action_token, 10))
            {
                DebugLog(_("action_token is not int:") + action_token_str + _T(";"));
            }
            else
            {
                DebugLog(wxString::Format(_T("parsed token(%d:%010d) (%s:%s)"), action_token, cmd_token,
                                         action_token_str.c_str(), cmd_token_str.c_str()));

                ResultParser *parser = new ResultParser;
                parser->Parse(line, result_type);

                if(parser->GetResultType() != dbg_mi::ResultParser::Result)
                {
                    DebugLog(_T("sending parser event"));

                    NotificationEvent event(parser);
                    //event.SetEventObject(this);
                    Manager::Get()->GetAppWindow()->GetEventHandler()->ProcessEvent(event);
                    delete parser;
                }
                else
                {
                    int64_t full_token = (static_cast<int64_t>(action_token) << 32) + cmd_token;
                    CommandMap::iterator it = m_active_commands.find(full_token);

                    if(it != m_active_commands.end())
                    {
                        ActionMap::iterator it_action = m_active_actions.find(action_token);
                        if(it_action != m_active_actions.end())
                        {
                            it_action->second->SetCommandResult(cmd_token, parser);
                        }
                        else
                        {
                            it->second->SetResult(parser);
                            DebugLog(parser->MakeDebugString());
                        }

                    }
                    else
                    {
                        wxString full_token_str;
                        full_token_str.Printf(_T("full token: %ld (%d%10d)\n"),
                                              full_token, full_token >> 32, full_token & 0xffffffff);

                        DebugLog(_("token not found:") + action_token_str + _T(":") + cmd_token_str + _T(";")
                                 + full_token_str);
                    }
                }
            }
        }
    }
    m_full_output.Remove(0, last_end);
}

void CommandQueue::AddAction(Action *action)
{
    action->SetCommandQueue(this);
    action->SetID(m_last_action_id++);
    m_active_actions[action->GetID()] = action;
    action->Start();
}

void CommandQueue::RemoveAction(Action *action)
{
    m_active_actions.erase(action->GetID());
    delete action;
}

void CommandQueue::Log(wxString const & s)
{
    LogManager &log = *Manager::Get()->GetLogManager();
    log.Log(s, m_normal_log);
}

void CommandQueue::DebugLog(wxString const & s)
{
    LogManager &log = *Manager::Get()->GetLogManager();
    log.Log(s, m_debug_log);
}

} // namespace dbg_mi
