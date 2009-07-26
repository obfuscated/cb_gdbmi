#include "actions.h"

#include <logmanager.h>

#include "cmd_result_parser.h"

namespace dbg_mi
{


WatchAction::WatchAction(wxString const &variable_name, int debug_page) :
    m_variable_name(variable_name),
    m_debug_page(debug_page)
{
}

void WatchAction::Start()
{
    QueueCommand(_T("-var-create ") + m_variable_name + _T("_gdb_var * ") + m_variable_name);
}

void WatchAction::OnCommandResult(int32_t cmd_id)
{
    LogManager &log = *Manager::Get()->GetLogManager();

    log.Log(wxString::Format(_T("WatchAction::OnCommandResult: %d"), cmd_id), m_debug_page);

    ResultParser *result = GetCommandResult(cmd_id);

    assert(result);

    log.Log(result->MakeDebugString(), m_debug_page);

    if(cmd_id == 0)
        QueueCommand(_T("-var-info-type ") + m_variable_name + _T("_gdb_var"));
    else if(cmd_id == 1)
    {
        Finish();
        return;
    }
}
} // namespace dbg_mi
