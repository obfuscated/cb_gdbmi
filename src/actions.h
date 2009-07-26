#ifndef _DEBUGGER_GDB_MI_ACTIONS_H_
#define _DEBUGGER_GDB_MI_ACTIONS_H_

#include "cmd_queue.h"

namespace dbg_mi
{

class WatchAction : public Action
{
public:
    WatchAction(wxString const &variable_name, int debug_page);

    virtual void Start();
    virtual void OnCommandResult(int32_t cmd_id);
private:
    wxString m_variable_name;

    int m_debug_page;
};

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_ACTIONS_H_
