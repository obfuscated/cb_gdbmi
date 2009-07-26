#ifndef _DEBUGGER_MI_GDB_CMD_QUEUE_H_
#define _DEBUGGER_MI_GDB_CMD_QUEUE_H_

#include <wx/thread.h>
#include <wx/string.h>

#include <deque>
#include <tr1/unordered_map>


class PipedProcess;

namespace dbg_mi
{

class ResultParser;

class Command
{
public:
    Command() :
        m_id(0),
        m_action_id(0),
        m_result(NULL)
    {
    }
    virtual ~Command();

    virtual void ParseOutput();

public:
    void SetID(int32_t id) { m_id = id; }
    int32_t GetID() const { return m_id; }

    void SetActionID(int32_t id) { m_action_id = id; }
    int32_t GetActionID() const { return m_action_id; }

    int64_t GetFullID() const { return (static_cast<int64_t>(m_action_id) << 32) + m_id; }

    void SetString(wxString const & command) { m_string = command; }
    wxString const & GetString() const { return m_string; }

    void SetResult(ResultParser *result) { m_result = result; }
private:
    wxString m_string;
    int32_t m_id;
    int32_t m_action_id;
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

    void SetCommandQueue(CommandQueue *queue) { m_queue = queue; }

    virtual void Start() = 0;
    virtual void OnCommandResult(int32_t cmd_id) = 0;

private:
    typedef std::tr1::unordered_map<int32_t, ResultParser*> CommandResult;

private:
    CommandQueue    *m_queue;
    CommandResult   m_command_results;
    int32_t m_id;
    int32_t m_last_cmd_id;
    bool m_started;
};


class CommandQueue
{
public:
    CommandQueue();
    ~CommandQueue();

    void SetDebugLogPageIndex(int debuglog_page_index) { m_debuglog_page_index = debuglog_page_index; }

    int64_t AddCommand(Command *command, bool generate_id);
    void RunQueue(PipedProcess *process);
    void AccumulateOutput(wxString const &output);

    void AddAction(Action *action);
    void RemoveAction(Action *action);

private:
    void ParseOutput();
private:
    typedef std::deque<Command*> Queue;
    typedef std::tr1::unordered_map<int64_t, Command*> CommandMap;
    typedef std::tr1::unordered_map<int32_t, Action*> ActionMap;

    Queue   m_commands_to_execute;
    CommandMap  m_active_commands;
    ActionMap   m_active_actions;
    int64_t m_last_cmd_id;
    int32_t m_last_action_id;
    wxMutex m_lock;
    int     m_debuglog_page_index;
    wxString    m_full_output;
};

} // namespace dbg_mi

#endif // _DEBUGGER_MI_GDB_CMD_QUEUE_H_
