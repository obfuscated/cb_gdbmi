#ifndef _DEBUGGER_MI_GDB_CMD_QUEUE_H_
#define _DEBUGGER_MI_GDB_CMD_QUEUE_H_


#include <deque>
#include <ostream>
#include <tr1/utility>
#include <tr1/unordered_map>


#include <wx/string.h>
/*
#include <wx/thread.h>



class PipedProcess;
*/

namespace dbg_mi
{
/*
class ResultParser;

class Command
{
public:
    Command() :
        m_id(0),
        m_action_id(0),
        m_wait_for_action(-1),
        m_result(NULL)
    {
    }
    ~Command();

//    virtual void ParseOutput();

public:
    void SetID(int32_t id) { m_id = id; }
    int32_t GetID() const { return m_id; }

    void SetActionID(int32_t id) { m_action_id = id; }
    int32_t GetActionID() const { return m_action_id; }

    int64_t GetFullID() const { return (static_cast<int64_t>(m_action_id) << 32) + m_id; }

    void SetWaitForAction(int32_t id) { m_wait_for_action = id; }
    int32_t GetWaitForAction() const { return m_wait_for_action; }

    void SetString(wxString const & command) { m_string = command; }
    wxString const & GetString() const { return m_string; }

    void SetResult(ResultParser *result) { m_result = result; }
private:
    wxString m_string;
    int32_t m_id;
    int32_t m_action_id;
    int32_t m_wait_for_action;
    ResultParser *m_result;
};

class CommandQueue;


class Action
{
public:
    Action() :
        m_queue(NULL),
        m_id(-1),
        m_last_cmd_id(0),
        m_wait_for_action(-1),
        m_started(false)
    {
    }
    virtual ~Action() {}


    void MarkStarted();
    int32_t QueueCommand(wxString const &cmd_string);
    void Finish();

    void SetCommandResult(int32_t cmd_id, ResultParser *parser);
    ResultParser* GetCommandResult(int32_t cmd_id);

    void SetID(int32_t id) { m_id = id; }
    int32_t GetID() const { return m_id; }

    void SetWaitForAction(int32_t id) { m_wait_for_action = id; }
    int32_t GetWaitForAction() const { return m_wait_for_action; }

    void SetCommandQueue(CommandQueue *queue) { m_queue = queue; }

#ifdef TEST_PROJECT
    CommandQueue* GetCommandQueue() const { return m_queue; }
#endif
    virtual void Start() = 0;
    virtual void OnCommandResult(int32_t cmd_id) = 0;
protected:
    void Log(wxString const &s);
    void DebugLog(wxString const &s);

private:
    typedef std::tr1::unordered_map<int32_t, ResultParser*> CommandResult;

private:
    CommandQueue    *m_queue;
    CommandResult   m_command_results;
    int32_t m_id;
    int32_t m_last_cmd_id;
    int32_t m_wait_for_action;
    bool m_started;
};


class CommandQueue
{
public:
    enum ExecutionType
    {
        Asynchronous,
        Synchronous
    };
public:
    CommandQueue();
    ~CommandQueue();

    void SetLogPages(int normal, int debug) { m_normal_log = normal; m_debug_log = debug; }

    int64_t AddCommand(Command *command, bool generate_id);
    void RunQueue(PipedProcess *process);
    void AccumulateOutput(wxString const &output);

    void AddAction(Action *action, ExecutionType exec_type);
    void RemoveAction(Action *action);

    void Log(wxString const & s);
    void DebugLog(wxString const & s);

private:
    void ParseOutput();
private:
    typedef std::deque<Command*> Queue;
    typedef std::tr1::unordered_map<int64_t, Command*> CommandMap;
//    typedef std::tr1::unordered_map<int32_t, Action*> ActionContainer;
    typedef std::deque<Action*> ActionContainer;

    Queue           m_commands_to_execute;
    CommandMap      m_active_commands;
    ActionContainer m_active_actions;
    int64_t         m_last_cmd_id;
    int32_t         m_last_action_id;
    wxMutex         m_lock;
    int             m_normal_log, m_debug_log;
    wxString        m_full_output;
};
*/

class ResultParser;

class CommandID
{
public:
    CommandID(int32_t action = -1, int32_t command_in_action = -1) :
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

private:
    int32_t m_action, m_command_in_action;
};

inline std::ostream& operator<< (std::ostream& s, CommandID const &id)
{
    s << id.ToString().utf8_str().data();
    return s;
}

struct CommandIDHash
{
    std::size_t operator()(CommandID const &id) const
    {
        return 0;
    }
};

bool ParseGDBOutputLine(wxString const &line, CommandID &id, wxString &result_str);

class Action
{
public:
    typedef std::deque<wxString> PendingCommands;
public:
    Action() :
        m_started(false),
        m_finished(false)
    {
    }

    virtual ~Action() {}

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

    void Execute(wxString const &command)
    {
        m_pending_commands.push_back(command);
    }

    int GetPendingCommandsCount() const { return m_pending_commands.size(); }
    bool HasPendingCommands() const { return !m_pending_commands.empty(); }

    wxString PopPendingCommand()
    {
        assert(HasPendingCommands());
        wxString cmd = m_pending_commands.front();
        m_pending_commands.pop_front();
        return cmd;
    }


public:
    virtual void OnCommandOutput(CommandID const &id, wxString const &output) = 0;
protected:
    virtual void OnStart() = 0;
private:
    PendingCommands m_pending_commands;
    bool m_started;
    bool m_finished;
};

class CommandExecutor
{
//    CommandExecutor const& operator=(CommandExecutor const &);
//    CommandExecutor(CommandExecutor const &);
public:
//    CommandExecutor() {}
    virtual ~CommandExecutor() {}

    CommandID Execute(wxString const &cmd)
    {
        return DoExecute(cmd);
    }

    virtual wxString GetOutput() = 0;
    virtual bool HasOutput() const = 0;
    virtual bool ProcessOutput(wxString const &output) = 0;

    virtual ResultParser* GetResult(CommandID &id) = 0;
protected:
    virtual CommandID DoExecute(wxString const &cmd) = 0;

};

class ActionsMap
{
public:
    ~ActionsMap();
    void Add(Action *action);
    bool Empty() const { return m_actions.empty(); }
    void Run(CommandExecutor &executor);
private:
    typedef std::deque<Action*> Actions;

    Actions m_actions;
};

class CommandResultMap
{
public:
    ~CommandResultMap();
    bool Set(CommandID const &id, ResultParser *parser);
    int GetCount() const;
    bool HasResult(CommandID const &id) const;
private:
    typedef std::tr1::unordered_map<CommandID, ResultParser*, CommandIDHash> Map;
    Map m_map;
};

bool ProcessOutput(CommandExecutor &executor, CommandResultMap &result_map);


} // namespace dbg_mi


#endif // _DEBUGGER_MI_GDB_CMD_QUEUE_H_
