#include "actions.h"

#include <cbplugin.h>
#include <logmanager.h>

#include <backtracedlg.h>
#include "cmd_result_parser.h"
#include "frame.h"

namespace dbg_mi
{

void BreakpointAddAction::OnStart()
{
    wxString cmd(wxT("-break-insert "));
    cbBreakpoint &bp = m_breakpoint->Get();

    if(bp.UseCondition())
        cmd += wxT("-c ") + bp.GetCondition() + wxT(" ");
    if(bp.UseIgnoreCount())
        cmd += wxT("-i ") + wxString::Format(wxT("%d "), bp.GetIgnoreCount());

    cmd += wxString::Format(wxT("%s:%d"), bp.GetFilename().c_str(), bp.GetLine());
    m_initial_cmd = Execute(cmd);
    m_logger.Debug(wxT("BreakpointAddAction::m_initial_cmd = ") + m_initial_cmd.ToString());
}

void BreakpointAddAction::OnCommandOutput(CommandID const &id, ResultParser const &result)
{
    m_logger.Debug(wxT("BreakpointAddAction::OnCommandResult: ") + id.ToString());

    if(m_initial_cmd == id)
    {
        const ResultValue &value = result.GetResultValue();
        const ResultValue *number = value.GetTupleValue(wxT("bkpt.number"));
        if(number)
        {
            const wxString &number_value = number->GetSimpleValue();
            long n;
            if(number_value.ToLong(&n, 10))
            {
                m_logger.Debug(wxString::Format(wxT("BreakpointAddAction::breakpoint index is %d"), n));
                m_breakpoint->SetIndex(n);

                if(!m_breakpoint->Get().IsEnabled())
                    m_disable_cmd = Execute(wxString::Format(wxT("-break-disable %d"), n));
                else
                {
                    m_logger.Debug(wxT("BreakpointAddAction::Finishing1"));
                    Finish();
                }
            }
            else
                m_logger.Debug(wxT("BreakpointAddAction::error getting the index :( "));
        }
        else
        {
            m_logger.Debug(wxT("BreakpointAddAction::error getting number value:( "));
            m_logger.Debug(value.MakeDebugString());
        }
    }
    else if(m_disable_cmd == id)
    {
        m_logger.Debug(wxT("BreakpointAddAction::Finishing2"));
        Finish();
    }
}

GenerateBacktrace::GenerateBacktrace(BacktraceContainer &backtrace, Logger &logger) :
    m_backtrace(backtrace),
    m_logger(logger),
    m_parsed_backtrace(false),
    m_parsed_args(false)
{
}
void GenerateBacktrace::OnCommandOutput(CommandID const &id, ResultParser const &result)
{
    if(id == m_backtrace_id)
    {
        ResultValue const *stack = result.GetResultValue().GetTupleValue(wxT("stack"));
        if(!stack)
            m_logger.Debug(wxT("GenerateBacktrace::OnCommandOutput: no stack tuple in the output"));
        else
        {
            int size = stack->GetTupleSize();
            m_logger.Debug(wxString::Format(wxT("GenerateBacktrace::OnCommandOutput: tuple size %d %s"),
                                            size, stack->MakeDebugString().c_str()));

            m_backtrace.clear();

            for(int ii = 0; ii < size; ++ii)
            {
                ResultValue const *frame_value = stack->GetTupleValueByIndex(ii);
                assert(frame_value);
                Frame frame;
                if(frame.ParseFrame(*frame_value))
                {
                    cbStackFrame s;
                    s.SetFile(frame.GetFilename(), wxString::Format(wxT("%d"), frame.GetLine()));
                    s.SetSymbol(frame.GetFunction());
                    s.SetNumber(ii);
                    s.SetAddress(frame.GetAddress());
                    s.MakeValid(frame.HasValidSource());
                    m_backtrace.push_back(s);
                }
                else
                    m_logger.Debug(wxT("can't parse frame: ") + frame_value->MakeDebugString());
            }
        }
        m_parsed_backtrace = true;
    }
    else if(id == m_args_id)
    {
        m_logger.Debug(wxT("GenerateBacktrace::OnCommandOutput arguments"));
        dbg_mi::FrameArguments arguments;

        if(!arguments.Attach(result.GetResultValue()))
        {
            m_logger.Debug(wxT("GenerateBacktrace::OnCommandOutput: can't attach to output of command: ")
                           + id.ToString());
        }
        else if(arguments.GetCount() != static_cast<int>(m_backtrace.size()))
        {
            m_logger.Debug(wxT("GenerateBacktrace::OnCommandOutput: stack arg count differ from the number of frames"));
        }
        else
        {
            int size = arguments.GetCount();
            for(int ii = 0; ii < size; ++ii)
            {
                wxString args;
                if(arguments.GetFrame(ii, args))
                    m_backtrace[ii].SetSymbol(m_backtrace[ii].GetSymbol() + wxT("(") + args + wxT(")"));
                else
                {
                    m_logger.Debug(wxString::Format(wxT("GenerateBacktrace::OnCommandOutput: ")
                                                    wxT("can't get args for frame %d"),
                                                    ii));
                }
            }
        }
        m_parsed_args = true;
    }

    if(m_parsed_backtrace && m_parsed_args)
    {
        Manager::Get()->GetDebuggerManager()->GetBacktraceDialog()->Reload();
        Finish();
    }
}
void GenerateBacktrace::OnStart()
{
    m_backtrace_id = Execute(wxT("-stack-list-frames 0 30"));
    m_args_id = Execute(wxT("-stack-list-arguments 1 0 30"));
}

/*
WatchAction::WatchAction(wxString const &variable_name) :
    m_variable_name(variable_name)
{
}

void WatchAction::Start()
{
    QueueCommand(wxT("-var-create ") + m_variable_name + wxT("_gdb_var * ") + m_variable_name);
}

void WatchAction::OnCommandResult(int32_t cmd_id)
{
    DebugLog(wxString::Format(wxT("WatchAction::OnCommandResult: %d"), cmd_id));

    ResultParser *result = GetCommandResult(cmd_id);
    assert(result);

    DebugLog(result->MakeDebugString());

    if(cmd_id == 0)
        QueueCommand(wxT("-var-info-type ") + m_variable_name + wxT("_gdb_var"));
    else if(cmd_id == 1)
    {
        Finish();
        return;
    }
}
*/
} // namespace dbg_mi
