#include "actions.h"

#include <cbplugin.h>
#include <logmanager.h>

#include "cmd_result_parser.h"
#include "definitions.h"

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
