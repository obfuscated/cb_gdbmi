#ifndef _DEBUGGER_GDB_MI_DEFINITIONS_H_
#define _DEBUGGER_GDB_MI_DEFINITIONS_H_

#include <deque>
#include <tr1/memory>

#include <wx/sizer.h>

#include <debuggermanager.h>
#include <scrollingdialog.h>

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
        m_symbol(symbol),
        m_has_been_expanded(false)
    {
    }

    void Reset()
    {
        m_id = m_type = m_value = wxEmptyString;
        m_has_been_expanded = false;

        RemoveChildren();
        Expand(false);
    }

    wxString const & GetID() const { return m_id; }
    void SetID(wxString const &id) { m_id = id; }

    bool HasBeenExpanded() const { return m_has_been_expanded; }
    void SetHasBeenExpanded(bool expanded) { m_has_been_expanded = expanded; }
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
    bool m_has_been_expanded;
};

typedef std::vector<dbg_mi::Watch::Pointer> WatchesContainer;

Watch* FindWatch(wxString const &expression, WatchesContainer &watches);

// Custom window to display output of DebuggerInfoCmd
class TextInfoWindow : public wxScrollingDialog
{
    public:
        TextInfoWindow(wxWindow *parent, const wxChar *title, const wxString& content) :
            wxScrollingDialog(parent, -1, title, wxDefaultPosition, wxDefaultSize,
                              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX)
        {
            wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
            wxFont font(8, wxMODERN, wxNORMAL, wxNORMAL);
            m_text = new wxTextCtrl(this, -1, content, wxDefaultPosition, wxDefaultSize,
                                    wxTE_READONLY | wxTE_MULTILINE | wxTE_RICH2 | wxHSCROLL);
            m_text->SetFont(font);

            sizer->Add(m_text, 1, wxGROW);

            SetSizer(sizer);
            sizer->Layout();
        }
    private:
        wxTextCtrl* m_text;
};

class CurrentFrame
{
public:
    CurrentFrame() :
        m_line(-1),
        m_stack_frame(-1),
        m_user_selected_stack_frame(-1),
        m_thread(-1)
    {
    }

    void Reset()
    {
        m_stack_frame = -1;
        m_user_selected_stack_frame = -1;
    }

    void SwitchToFrame(int frame_number)
    {
        m_user_selected_stack_frame = m_stack_frame = frame_number;
    }

    void SetFrame(int frame_number)
    {
        if (m_user_selected_stack_frame >= 0)
            m_stack_frame = m_user_selected_stack_frame;
        else
            m_stack_frame = frame_number;
    }
    void SetThreadId(int thread_id) { m_thread = thread_id; }
    void SetPosition(wxString const &filename, int line)
    {
        m_filename = filename;
        m_line = line;
    }

    int GetStackFrame() const { return m_stack_frame; }
    int GetUserSelectedFrame() const { return m_user_selected_stack_frame; }
    void GetPosition(wxString &filename, int &line)
    {
        filename = m_filename;
        line = m_line;
    }
    int GetThreadId() const { return m_thread; }

private:
    wxString m_filename;
    int m_line;
    int m_stack_frame;
    int m_user_selected_stack_frame;
    int m_thread;
};

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_DEFINITIONS_H_
