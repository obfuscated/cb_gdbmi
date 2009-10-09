#ifndef _DEBUGGER_GDB_MI_ACTIONS_H_
#define _DEBUGGER_GDB_MI_ACTIONS_H_

#include <tr1/memory>
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
/*
class WatchAction : public Action
{
public:
    WatchAction(wxString const &variable_name);

    virtual void Start();
    virtual void OnCommandResult(int32_t cmd_id);
private:
    wxString m_variable_name;
};
*/
} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_ACTIONS_H_
