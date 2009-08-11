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

CommandQueue::CommandQueue() :
    m_last_cmd_id(0),
    m_last_action_id(1) // action_id == 0 is reserved for actionless commands
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
    Manager::Get()->GetLogManager()->LogWarning(_("adding commnad: ") + full_cmd, m_debuglog_page_index);

    m_commands_to_execute.push_back(command);
    return command->GetFullID();
}
void CommandQueue::RunQueue(PipedProcess *process)
{
    LogManager &log = *Manager::Get()->GetLogManager();

    Queue executed_cmds;
    while(!m_commands_to_execute.empty())
    {
        Command *command = m_commands_to_execute.front();
        m_commands_to_execute.pop_front();

        wxString full_cmd = wxString::Format(_T("%d%010d"), command->GetActionID(), command->GetID())
                            + command->GetString();

        log.Log(_("sending commnd: ") + full_cmd, m_debuglog_page_index);
        process->SendString(full_cmd);

        executed_cmds.push_back(command);
    }

    {
        wxMutexLocker locker(m_lock);
        for(Queue::iterator it = executed_cmds.begin(); it != executed_cmds.end(); ++it)
            m_active_commands[(*it)->GetID()] = *it;
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
    LogManager &log = *Manager::Get()->GetLogManager();
//    log.Log(_(""), m_debuglog_page_index);
//    log.Log(_("full output --->> ") + m_full_output + _("<<---"), m_debuglog_page_index);
//    log.Log(_(""), m_debuglog_page_index);

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
            log.Log(wxString::Format(_("status -> start: %d; end: %d; token_start: %d"), start, end, token_start),
                    m_debuglog_page_index);
            wxString token_str = m_full_output.substr(token_start, start - token_start);
            wxString line = m_full_output.substr(start + 1, end - start - 1);
            log.Log(_("token:") + token_str + _(";parsed line >>: ") + line + _(":<<"), m_debuglog_page_index);

            last_end = end;
            start = end = -1;
            token_start = -1;

            wxString cmd_token_str = token_str.substr(std::max(token_str.length() - 10, 0lu), 10);
            wxString action_token_str = token_str.substr(0, std::max(token_str.length() - 10, 0lu));

            long cmd_token, action_token;

            if(!cmd_token_str.ToLong(&cmd_token, 10))
            {
                log.Log(_("cmd_token is not int:") + cmd_token_str + _T(";"), m_debuglog_page_index);
            }
            else if(!action_token_str.ToLong(&action_token, 10))
            {
                log.Log(_("action_token is not int:") + action_token_str + _T(";"), m_debuglog_page_index);
            }
            else
            {
                log.Log(wxString::Format(_T("token(%d:%010d) (%s:%s)"), action_token, cmd_token,
                                         action_token_str.c_str(), cmd_token_str.c_str()), m_debuglog_page_index);

                ResultParser *parser = new ResultParser;
                parser->Parse(line, result_type);

                if(parser->GetResultType() != dbg_mi::ResultParser::Result)
                {
                    log.Log(_T("sending parser event"), m_debuglog_page_index);

                    NotificationEvent event(parser);
                    //event.SetEventObject(this);
                    Manager::Get()->GetAppWindow()->GetEventHandler()->ProcessEvent(event);
                    delete parser;
                }
                else
                {
                    int64_t full_token = (static_cast<int64_t>(action_token) << 32) + cmd_token;
                    CommandMap::iterator it = m_active_commands.find(action_token);

                    if(it != m_active_commands.end())
                    {
//                        it->second->SetResult(parser);
                        ActionMap::iterator it_action = m_active_actions.find(action_token);
                        if(it_action != m_active_actions.end())
                        {
                            it_action->second->SetCommandResult(cmd_token, parser);
                        }
                        else
                        {
                            it->second->SetResult(parser);
                            log.Log(parser->MakeDebugString(), m_debuglog_page_index);
                        }

                    }
                    else
                    {
                        log.Log(_("token not found:") + action_token_str + _T(":") + cmd_token_str + _T(";")
                                + wxString::Format(_T("full token: %ld\n"), full_token), m_debuglog_page_index);
                    }
                }
            }
        }
    }
    m_full_output.Remove(0, last_end);
//    log.Log(_(""), m_debuglog_page_index);
}

void CommandQueue::AddAction(Action *action)
{
    action->SetCommandQueue(this);
    action->SetID(m_last_action_id++);
    m_active_actions[action->GetID()] = action;

//    LogManager &log = *Manager::Get()->GetLogManager();
//    log.Log(wxString::Format(_T("Adding action: %d"), action->GetID()), m_debuglog_page_index);

    action->Start();
}

void CommandQueue::RemoveAction(Action *action)
{
    m_active_actions.erase(action->GetID());
    delete action;
}

} // namespace dbg_mi
