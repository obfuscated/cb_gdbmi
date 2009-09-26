#include "cmd_queue.h"

/*
#include <algorithm>
#ifndef TEST_PROJECT
#   include <logmanager.h>
#   include <pipedprocess.h>
#endif

#include "cmd_result_parser.h"
#include "events.h"
*/

namespace dbg_mi
{
/*
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
    cmd->SetWaitForAction(GetWaitForAction());

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
    for(ActionContainer::iterator it = m_active_actions.begin(); it != m_active_actions.end(); ++it)
    //    delete it->second;
        delete *it;
}

int64_t CommandQueue::AddCommand(Command *command, bool generate_id)
{
    if(generate_id)
        command->SetID(m_last_cmd_id++);

    wxString full_cmd = wxString::Format(wxT("%d (last: %d)"), command->GetID(), m_last_cmd_id) + command->GetString();
    DebugLog(wxT("adding commnad: ") + full_cmd);

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

        DebugLog(wxT("sending commnd: ") + full_cmd);
#ifndef TEST_PROJECT
        process->SendString(full_cmd);
#endif

        executed_cmds.push_back(command);
    }

    {
        wxMutexLocker locker(m_lock);
        for(Queue::iterator it = executed_cmds.begin(); it != executed_cmds.end(); ++it)
        {
            DebugLog(wxString::Format(wxT("Adding active command %010ld (%d:%010d)"),
                                      (*it)->GetFullID(), (*it)->GetActionID(), (*it)->GetID()));
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
            DebugLog(wxT("token:") + token_str + wxT(";parsed line >>: ") + line + wxT(":<<"));

            last_end = end;
            start = end = -1;
            token_start = -1;

            wxString cmd_token_str = token_str.substr(std::max(token_str.length() - 10, 0lu), 10);
            wxString action_token_str = token_str.substr(0, std::max(token_str.length() - 10, 0lu));

            long cmd_token, action_token;

            if(!cmd_token_str.ToLong(&cmd_token, 10))
            {
                DebugLog(wxT("cmd_token is not int:") + cmd_token_str + wxT(";"));
            }
            else if(!action_token_str.ToLong(&action_token, 10))
            {
                DebugLog(wxT("action_token is not int:") + action_token_str + wxT(";"));
            }
            else
            {
                DebugLog(wxString::Format(wxT("parsed token(%d:%010d) (%s:%s)"), action_token, cmd_token,
                                         action_token_str.c_str(), cmd_token_str.c_str()));

                ResultParser *parser = new ResultParser;
                parser->Parse(line, result_type);

                if(parser->GetResultType() != dbg_mi::ResultParser::Result)
                {
                    DebugLog(wxT("sending parser event"));

#ifndef TEST_PROJECT
                    NotificationEvent event(parser);
                    Manager::Get()->GetAppWindow()->GetEventHandler()->ProcessEvent(event);
#endif
                    delete parser;
                }
                else
                {
                    int64_t full_token = (static_cast<int64_t>(action_token) << 32) + cmd_token;
                    CommandMap::iterator it = m_active_commands.find(full_token);

                    if(it != m_active_commands.end())
                    {
                        ActionContainer::iterator it_action;
                        for(it_action = m_active_actions.begin(); it_action != m_active_actions.end(); ++it_action)
                        {
                            DebugLog(wxString::Format(wxT("testing: %d == %d"), (*it_action)->GetID(), action_token));
                            if((*it_action)->GetID() == action_token)
                                break;
                        }
                        if(it_action != m_active_actions.end())
                        {
                            DebugLog(wxString::Format(wxT("found action %d"), action_token));
                            (*it_action)->SetCommandResult(cmd_token, parser);
                        }
                        else
                        {
                            DebugLog(wxString::Format(wxT("found no action %d"), action_token));
                            it->second->SetResult(parser);
                            DebugLog(parser->MakeDebugString());
                        }
                    }
                    else
                    {
                        wxString full_token_str;
                        full_token_str.Printf(wxT("full token: %ld (%d%10d)\n"),
                                              full_token, full_token >> 32, full_token & 0xffffffff);

                        DebugLog(wxT("token not found:") + action_token_str + wxT(":") + cmd_token_str + wxT(";")
                                 + full_token_str);
                    }
                }
            }
        }
    }
    m_full_output.Remove(0, last_end);
}

void CommandQueue::AddAction(Action *action, ExecutionType exec_type)
{
    if(exec_type == Synchronous)
        action->SetWaitForAction(m_last_action_id);
    action->SetCommandQueue(this);
    action->SetID(m_last_action_id++);
    m_active_actions[action->GetID()] = action;
    action->Start();
}

void CommandQueue::RemoveAction(Action *action)
{
    ActionContainer::iterator it_action;
    for(it_action = m_active_actions.begin(); it_action != m_active_actions.end(); ++it_action)
    {
        if(*it_action == action)
        {
            DebugLog(wxString::Format(wxT("Removing action: %d"), action->GetID()));
            m_active_actions.erase(it_action);
            delete action;
            return;
        }
    }
}

void CommandQueue::Log(wxString const & s)
{
#ifndef TEST_PROJECT
    LogManager &log = *Manager::Get()->GetLogManager();
    log.Log(s, m_normal_log);
#endif
}

void CommandQueue::DebugLog(wxString const & s)
{
#ifndef TEST_PROJECT
    LogManager &log = *Manager::Get()->GetLogManager();
    log.Log(s, m_debug_log);
#endif
}
*/
bool ParseGDBOutputLine(wxString const &line, CommandID &id, wxString &result_str)
{
    size_t pos = 0;
    while(pos < line.length() && wxIsdigit(line[pos]))
        ++pos;
    if(pos <= 10)
        return false;
    long action_id, cmd_id;

    wxString const &str_action = line.substr(0, pos - 10);
    str_action.ToLong(&action_id, 10);

    wxString const &str_cmd = line.substr(pos - 10, 10);
    str_cmd.ToLong(&cmd_id, 10);

    id = dbg_mi::CommandID(action_id, cmd_id);
    result_str = line.substr(pos, line.length() - pos);
    return true;
}

CommandResultMap::~CommandResultMap()
{
    for(Map::iterator it = m_map.begin(); it != m_map.end(); ++it)
        delete it->second;
}
bool CommandResultMap::Set(CommandID const &id, ResultParser *parser)
{
    if(HasResult(id))
        return false;
    else
    {
        m_map[id] = parser;
        return true;
    }
}
int CommandResultMap::GetCount() const
{
    return m_map.size();
}
bool CommandResultMap::HasResult(CommandID const &id) const
{
    return m_map.find(id) != m_map.end();
}

bool ProcessOutput(CommandExecutor &executor, CommandResultMap &result_map)
{
    while(executor.HasOutput())
    {
        dbg_mi::CommandID result_id;
        dbg_mi::ResultParser *result = executor.GetResult(result_id);

        if(result)
        {
            if(!result_map.Set(result_id, result))
                delete result;
        }
        else
            return false;
    }
    return true;
}

ActionsMap::ActionsMap() :
    m_last_id(1)
{
}

ActionsMap::~ActionsMap()
{
    for(Actions::iterator it = m_actions.begin(); it != m_actions.end(); ++it)
        delete *it;
}

void ActionsMap::Add(Action *action)
{
    action->SetID(m_last_id++);
    m_actions.push_back(action);
}

Action* ActionsMap::Find(int id)
{
    for(Actions::iterator it = m_actions.begin(); it != m_actions.end(); ++it)
    {
        if((*it)->GetID() == id)
            return *it;
    }
    return NULL;
}

Action const * ActionsMap::Find(int id) const
{
    for(Actions::const_iterator it = m_actions.begin(); it != m_actions.end(); ++it)
    {
        if((*it)->GetID() == id)
            return *it;
    }
    return NULL;
}

void ActionsMap::Run(CommandExecutor &executor)
{
    if(Empty())
        return;
    for(Actions::iterator it = m_actions.begin(); it != m_actions.end(); ++it)
    {
        Action &action = **it;

        if(!action.Started())
        {
            action.Start();
        }
        while(action.HasPendingCommands())
        {
            CommandID id;
            wxString const &command = action.PopPendingCommand(id);
            executor.ExecuteSimple(id, command);
        }

        if(action.Finished())
        {
            Actions::iterator del_it = it;
            --it;
            delete *del_it;
            m_actions.erase(del_it);
        }
    }
}
} // namespace dbg_mi
