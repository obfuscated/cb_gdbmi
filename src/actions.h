#ifndef _DEBUGGER_GDB_MI_ACTIONS_H_
#define _DEBUGGER_GDB_MI_ACTIONS_H_

#include <tr1/memory>
#include <tr1/unordered_map>
#include "cmd_queue.h"
#include "definitions.h"

class cbDebuggerPlugin;

namespace dbg_mi
{

class SimpleAction : public Action
{
public:
    SimpleAction(wxString const &cmd) :
        m_command(cmd)
    {
    }

    virtual void OnCommandOutput(CommandID const &id, ResultParser const &result)
    {
        Finish();
    }
protected:
    virtual void OnStart()
    {
        Execute(m_command);
    }
private:
    wxString m_command;
};

class Breakpoint;

class BreakpointAddAction : public Action
{
public:
    BreakpointAddAction(std::tr1::shared_ptr<Breakpoint> const &breakpoint, Logger &logger) :
        m_breakpoint(breakpoint),
        m_logger(logger)
    {
    }
    virtual ~BreakpointAddAction()
    {
        m_logger.Debug(wxT("BreakpointAddAction::destructor"));
    }
    virtual void OnCommandOutput(CommandID const &id, ResultParser const &result);
protected:
    virtual void OnStart();

private:
    std::tr1::shared_ptr<Breakpoint> m_breakpoint;
    CommandID m_initial_cmd, m_disable_cmd;

    Logger &m_logger;
};

template<typename StopNotification>
class RunAction : public Action
{
public:
    RunAction(cbDebuggerPlugin *plugin, const wxString &command,
              StopNotification notification, Logger &logger) :
        m_plugin(plugin),
        m_command(command),
        m_notification(notification),
        m_logger(logger)
    {
        SetWaitPrevious(true);
    }
    virtual ~RunAction()
    {
        m_logger.Debug(wxT("RunAction::destructor"));
    }

    virtual void OnCommandOutput(CommandID const &id, ResultParser const &result)
    {
        if(result.GetResultClass() == ResultParser::ClassRunning)
        {
            m_logger.Debug(wxT("RunAction success, the debugger is !stopped!"));
            m_logger.Debug(wxT("RunAction::Output - ") + result.MakeDebugString());
            m_notification(false);
            Finish();
        }
    }
protected:
    virtual void OnStart()
    {
        Execute(m_command);
        m_logger.Debug(wxT("RunAction::OnStart -> ") + m_command);
    }

private:
    cbDebuggerPlugin *m_plugin;
    wxString m_command;
    StopNotification m_notification;
    Logger &m_logger;
};

class GenerateBacktrace : public Action
{
public:
    GenerateBacktrace(BacktraceContainer &backtrace, Logger &logger);
    virtual void OnCommandOutput(CommandID const &id, ResultParser const &result);
protected:
    virtual void OnStart();
private:
    BacktraceContainer &m_backtrace;
    Logger &m_logger;
    CommandID m_backtrace_id, m_args_id;
    bool m_parsed_backtrace, m_parsed_args;
};

class GenerateThreadsList : public Action
{
public:
    GenerateThreadsList(ThreadsContainer &threads, int current_thread_id, Logger &logger);
    virtual void OnCommandOutput(CommandID const &id, ResultParser const &result);
protected:
    virtual void OnStart();
private:
    ThreadsContainer &m_threads;
    Logger &m_logger;
    int m_current_thread_id;
};


template<typename Notification>
class SwitchToThread : public Action
{
public:
    SwitchToThread(int thread_id, Logger &logger, Notification const &notification) :
        m_thread_id(thread_id),
        m_logger(logger),
        m_notification(notification)
    {
    }

    virtual void OnCommandOutput(CommandID const &id, ResultParser const &result)
    {
        m_notification(result);
        Finish();
    }
protected:
    virtual void OnStart()
    {
        Execute(wxString::Format(wxT("-thread-select %d"), m_thread_id));
    }

private:
    int m_thread_id;
    Logger &m_logger;
    Notification m_notification;
};

class WatchBaseAction : public Action
{
public:
    WatchBaseAction(Watch::Pointer const &watch, Logger &logger);
    virtual ~WatchBaseAction();

protected:
    void ExecuteListCommand(Watch &watch, Watch *parent);
    bool ParseListCommand(CommandID const &id, ResultValue const &value);
    void AppendNullChild(Watch &watch);
protected:
    typedef std::tr1::unordered_map<CommandID, Watch*> ListCommandParentMap;
protected:
    Watch::Pointer m_watch;
    ListCommandParentMap m_parent_map;
    Logger &m_logger;
    int m_sub_commands_left;
};

class WatchCreateAction : public WatchBaseAction
{
    enum Step
    {
        StepCreate = 0,
        StepListChildren
    };
public:
    WatchCreateAction(Watch::Pointer const &watch, Logger &logger);

    virtual void OnCommandOutput(CommandID const &id, ResultParser const &result);
protected:
    virtual void OnStart();

private:
    Step m_step;
};

class WatchesUpdateAction : public Action
{
public:
    WatchesUpdateAction(WatchesContainer &watches, Logger &logger);
    virtual ~WatchesUpdateAction();

    virtual void OnCommandOutput(CommandID const &id, ResultParser const &result);
protected:
    virtual void OnStart();

private:
    WatchesContainer &m_watches;
    Logger &m_logger;
};

class WatchExpandedAction : public WatchBaseAction
{
public:
    WatchExpandedAction(Watch::Pointer parent_watch, Watch *expanded_watch, Logger &logger) :
        WatchBaseAction(parent_watch, logger),
        m_expanded_watch(expanded_watch)
    {
    }

    virtual void OnCommandOutput(CommandID const &id, ResultParser const &result);
protected:
    virtual void OnStart();

private:
    Watch *m_expanded_watch;
};

class WatchCollapseAction : public WatchBaseAction
{
public:
    WatchCollapseAction(Watch::Pointer parent_watch, Watch *collapsed_watch, Logger &logger) :
        WatchBaseAction(parent_watch, logger),
        m_collapsed_watch(collapsed_watch)
    {
    }

    virtual void OnCommandOutput(CommandID const &id, ResultParser const &result);
protected:
    virtual void OnStart();

private:
    Watch *m_collapsed_watch;
};

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_ACTIONS_H_
