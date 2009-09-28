#ifndef _DEBUGGER_MI_GDB_CMD_QUEUE_H_
#define _DEBUGGER_MI_GDB_CMD_QUEUE_H_


#include <deque>
#include <ostream>

#include <wx/string.h>

#include "cmd_result_parser.h"
/*
#include <wx/thread.h>
class PipedProcess;
*/

namespace dbg_mi
{

class CommandID
{
public:
    explicit CommandID(int32_t action = -1, int32_t command_in_action = -1) :
        m_action(action),
        m_command_in_action(command_in_action)
    {
    }

    bool operator ==(CommandID const &o) const
    {
        return m_action == o.m_action && m_command_in_action == o.m_command_in_action;
    }
    bool operator !=(CommandID const &o) const
    {
        return !(*this == o);
    }


    CommandID& operator++() // prefix
    {
        ++m_command_in_action;
        return *this;
    }

    CommandID operator++(int) // postfix
    {
        CommandID old = *this;
        ++m_command_in_action;
        return old;
    }

    wxString ToString() const
    {
        return wxString::Format(wxT("%d%010d"), m_action, m_command_in_action);
    }

    int32_t GetActionID() const
    {
        return m_action;
    }

    int32_t GetCommandID() const
    {
        return m_command_in_action;
    }

private:
    int32_t m_action, m_command_in_action;
};

inline std::ostream& operator<< (std::ostream& s, CommandID const &id)
{
    s << id.ToString().utf8_str().data();
    return s;
}

bool ParseGDBOutputLine(wxString const &line, CommandID &id, wxString &result_str);

class Action
{
    struct Command
    {
        Command() {}
        Command(wxString const &string_, int id_) :
            string(string_),
            id(id_)
        {
        }

        wxString string;
        int id;
    };
    typedef std::deque<Command> PendingCommands;
public:
    Action() :
        m_id(-1),
        m_last_command_id(0),
        m_started(false),
        m_finished(false),
        m_wait_previous(false)
    {
    }

    virtual ~Action() {}

    void SetID(int id) { m_id = id; }
    int GetID() const { return m_id; }

    void Start()
    {
        m_started = true;
        OnStart();
    }

    void Finish()
    {
        m_finished = true;
    }

    bool Started() const { return m_started; }
    bool Finished() const { return m_finished; }

    void SetWaitPrevious(bool flag) { m_wait_previous = flag; }
    bool GetWaitPrevious() const { return m_wait_previous; }

    CommandID Execute(wxString const &command)
    {
        m_pending_commands.push_back(Command(command, m_last_command_id));
        return CommandID(m_id, m_last_command_id++);
    }

    int GetPendingCommandsCount() const { return m_pending_commands.size(); }
    bool HasPendingCommands() const { return !m_pending_commands.empty(); }

    wxString PopPendingCommand(CommandID &id)
    {
        assert(HasPendingCommands());
        Command cmd = m_pending_commands.front();
        m_pending_commands.pop_front();

        id = CommandID(GetID(), cmd.id);
        return cmd.string;
    }


public:
    virtual void OnCommandOutput(CommandID const &id, ResultParser const &result) = 0;
protected:
    virtual void OnStart() = 0;
private:
    PendingCommands m_pending_commands;
    int m_id;
    int m_last_command_id;
    bool m_started;
    bool m_finished;
    bool m_wait_previous;
};

class CommandExecutor
{
public:
    struct Result
    {
        dbg_mi::CommandID id;
        wxString output;
    };
//    CommandExecutor const& operator=(CommandExecutor const &);
//    CommandExecutor(CommandExecutor const &);
public:
    CommandExecutor() : m_last(0) {}
    virtual ~CommandExecutor() {}

    CommandID Execute(wxString const &cmd)
    {
        dbg_mi::CommandID id(1, m_last++);
        if(DoExecute(id, cmd))
            return id;
        else
            return dbg_mi::CommandID();
    }

    void ExecuteSimple(dbg_mi::CommandID const &id, wxString const &cmd)
    {
        DoExecute(id, cmd);
    }

    virtual wxString GetOutput() = 0;

    bool HasOutput() const { return !m_results.empty(); }
    bool ProcessOutput(wxString const &output)
    {
        dbg_mi::CommandID id;
        Result r;

        if(!dbg_mi::ParseGDBOutputLine(output, r.id, r.output))
            return false;

        m_results.push_back(r);
        return true;
    }

    dbg_mi::ResultParser* GetResult(dbg_mi::CommandID &id)
    {
        assert(!m_results.empty());
        Result const &r = m_results.front();

        id = r.id;
        dbg_mi::ResultParser *parser = new dbg_mi::ResultParser;
        if(!parser->Parse(r.output))
        {
            delete parser;
            parser = NULL;
        }

        m_results.pop_front();
        return parser;
    }

protected:
    virtual bool DoExecute(dbg_mi::CommandID const &id, wxString const &cmd) = 0;

protected:
    typedef std::deque<Result> Results;
    Results m_results;
    int32_t m_last;
};

class ActionsMap
{
public:
    ActionsMap();
    ~ActionsMap();

    void Add(Action *action);
    Action* Find(int id);
    Action const * Find(int id) const;

    bool Empty() const { return m_actions.empty(); }
    void Run(CommandExecutor &executor);
private:
    typedef std::deque<Action*> Actions;

    Actions m_actions;
    int m_last_id;
};

template<typename OnNotify>
bool DispatchResults(CommandExecutor &exec, ActionsMap &actions_map, OnNotify &on_notify)
{
    while(exec.HasOutput())
    {
        CommandID id;
        ResultParser *parser = exec.GetResult(id);

        if(!parser)
            return false;

        switch(parser->GetResultType())
        {
        case ResultParser::Result:
            {
                Action *action = actions_map.Find(id.GetActionID());
                if(action)
                    action->OnCommandOutput(id, *parser);
            }
            break;
        case ResultParser::TypeUnknown:
            break;
        default:
            on_notify(*parser);
        }

        delete parser;
    }
    return true;
}

} // namespace dbg_mi


#endif // _DEBUGGER_MI_GDB_CMD_QUEUE_H_
