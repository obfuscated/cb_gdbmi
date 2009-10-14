#ifndef _DEBUGGER_GDB_MI_DEFINITIONS_H_
#define _DEBUGGER_GDB_MI_DEFINITIONS_H_

#include <tr1/memory>
#include <debuggermanager.h>

namespace dbg_mi
{

class Breakpoint
{
public:
    typedef std::tr1::shared_ptr<Breakpoint> Pointer;
public:
    Breakpoint() :
        m_index(-1)
    {
    }
    Breakpoint(cbBreakpoint breakpoint) :
        m_breakpoint(breakpoint),
        m_index(-1)
    {
    }

    cbBreakpoint& Get() { return m_breakpoint; }
    const cbBreakpoint& Get() const { return m_breakpoint; }
    int GetIndex() const { return m_index; }

    void SetIndex(int index) { m_index = index; }
private:
    cbBreakpoint m_breakpoint;
    int m_index;
};


typedef std::deque<cbStackFrame> BacktraceContainer;
typedef std::deque<cbThread> ThreadsContainer;

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_DEFINITIONS_H_
