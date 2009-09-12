#include "actions.h"

#include <logmanager.h>

#include "cmd_result_parser.h"
#include "definitions.h"

namespace dbg_mi
{
void BreakpointAddAction::Start()
{
    wxString cmd;
    cmd.Printf(wxT("-break-insert %s:%d"), m_breakpoint->Get().GetFilename().c_str(), m_breakpoint->Get().GetLine());
    QueueCommand(cmd);
}

void BreakpointAddAction::OnCommandResult(int32_t cmd_id)
{
    LogManager &log = *Manager::Get()->GetLogManager();

    log.Log(wxString::Format(wxT("BreakpointAddAction::OnCommandResult: %d"), cmd_id), m_debug_page);

    ResultParser *result = GetCommandResult(cmd_id);
    assert(result);
    const ResultValue &value = result->GetResultValue();
    const ResultValue *number = value.GetTupleValue(wxT("bkpt.number"));
    if(number)
    {
        const wxString &number_value = number->GetSimpleValue();
        long n;
        if(number_value.ToLong(&n, 10))
        {
            log.Log(wxString::Format(wxT("BreakpointAddAction::breakpoint index is %d"), n), m_debug_page);
            m_breakpoint->SetIndex(n);
        }
        else
            log.Log(wxT("BreakpointAddAction::error getting the index :( "), m_debug_page);
    }
    else
    {
        log.Log(wxT("BreakpointAddAction::error getting number value:( "), m_debug_page);
        log.Log(value.MakeDebugString(), m_debug_page);
    }
}

RunAction::RunAction(const wxString &command, bool &is_stopped) :
    m_command(command),
    m_is_stopped(is_stopped)
{
}

void RunAction::Start()
{
    QueueCommand(m_command);
}
void RunAction::OnCommandResult(int32_t cmd_id)
{
    ResultParser *result = GetCommandResult(cmd_id);
    assert(result);
    if(result->GetResultClass() == ResultParser::ClassDone)
        m_is_stopped = false;
}

WatchAction::WatchAction(wxString const &variable_name, int debug_page) :
    m_variable_name(variable_name),
    m_debug_page(debug_page)
{
}

void WatchAction::Start()
{
    QueueCommand(wxT("-var-create ") + m_variable_name + wxT("_gdb_var * ") + m_variable_name);
}

void WatchAction::OnCommandResult(int32_t cmd_id)
{
    LogManager &log = *Manager::Get()->GetLogManager();

    log.Log(wxString::Format(wxT("WatchAction::OnCommandResult: %d"), cmd_id), m_debug_page);

    ResultParser *result = GetCommandResult(cmd_id);

    assert(result);

    log.Log(result->MakeDebugString(), m_debug_page);

    if(cmd_id == 0)
        QueueCommand(wxT("-var-info-type ") + m_variable_name + wxT("_gdb_var"));
    else if(cmd_id == 1)
    {
        Finish();
        return;
    }
}
} // namespace dbg_mi
