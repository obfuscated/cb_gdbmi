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

class Watch : public cbWatch
{
public:
    typedef std::tr1::shared_ptr<Watch> Pointer;
public:
    Watch(wxString const &symbol) :
        m_symbol(symbol)
    {
    }

    wxString const & GetID() const { return m_id; }
    void SetID(wxString const &id) { m_id = id; }
public:
    virtual void GetSymbol(wxString &symbol) const { symbol = m_symbol; }
    virtual void GetValue(wxString &value) const { value = m_value; }
    virtual bool SetValue(const wxString &value) { m_value = value; return true; }
    virtual void GetFullWatchString(wxString &full_watch) const { full_watch = m_value; }
    virtual void GetType(wxString &type) const { type = m_type; }
    virtual void SetType(const wxString &type) { m_type = type; }

    virtual wxString const & GetDebugString() const
    {
        m_debug_string = m_id + wxT("->") + m_symbol + wxT(" = ") + m_value;
        return m_debug_string;
    }
protected:
    virtual void DoDestroy() {}
private:
    wxString m_id;
    wxString m_symbol;
    wxString m_value;
    wxString m_type;

    mutable wxString m_debug_string;
};

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_DEFINITIONS_H_
