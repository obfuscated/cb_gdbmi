#ifndef _DEBUGGER_GDB_MI_DEFINITIONS_H_
#define _DEBUGGER_GDB_MI_DEFINITIONS_H_

#include <deque>
#include <tr1/memory>

#include <wx/sizer.h>

#include <debuggermanager.h>
#include <scrollingdialog.h>

namespace dbg_mi
{

class Breakpoint : public cbBreakpoint
{
public:
    typedef std::tr1::shared_ptr<Breakpoint> Pointer;
public:
    Breakpoint() :
        m_project(nullptr),
        m_index(-1),
        m_line(-1),
        m_enabled(true),
        m_temporary(false)
    {
    }

    Breakpoint(const wxString &filename, int line, cbProject *project) :
        m_filename(filename),
        m_project(project),
        m_index(-1),
        m_line(line),
        m_enabled(true),
        m_temporary(false)
    {
    }

    virtual void SetEnabled(bool flag);
    virtual wxString GetLocation() const;
    virtual int GetLine() const;
    virtual wxString GetLineString() const;
    virtual wxString GetType() const;
    virtual wxString GetInfo() const;
    virtual bool IsEnabled() const;
    virtual bool IsVisibleInEditor() const;
    virtual bool IsTemporary() const;

    int GetIndex() const { return m_index; }
    const wxString& GetCondition() const { return m_condition; }
    int GetIgnoreCount() const { return 0; }

    bool HasCondition() const { return false; }
    bool HasIgnoreCount() const { return false; }

    void SetIndex(int index) { m_index = index; }

    cbProject* GetProject() { return m_project; }
private:
    wxString m_filename;
    wxString m_condition;
    cbProject *m_project;
    int m_index;
    int m_line;
    bool m_enabled;
    bool m_temporary;
};


typedef std::deque<cbStackFrame::Pointer> BacktraceContainer;
typedef std::deque<cbThread::Pointer> ThreadsContainer;

class Watch : public cbWatch
{
public:
    typedef std::tr1::shared_ptr<Watch> Pointer;
public:
    Watch(wxString const &symbol, bool for_tooltip) :
        m_symbol(symbol),
        m_has_been_expanded(false),
        m_for_tooltip(for_tooltip)
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
    bool ForTooltip() const { return m_for_tooltip; }
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
    bool m_for_tooltip;
};

typedef std::vector<dbg_mi::Watch::Pointer> WatchesContainer;

Watch::Pointer FindWatch(wxString const &expression, WatchesContainer &watches);

// Custom window to display output of DebuggerInfoCmd
class TextInfoWindow : public wxScrollingDialog
{
    public:
        TextInfoWindow(wxWindow *parent, const wxChar *title, const wxString& content) :
            wxScrollingDialog(parent, -1, title, wxDefaultPosition, wxDefaultSize,
                              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX),
            m_font(8, wxMODERN, wxNORMAL, wxNORMAL)
        {
            wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
            m_text = new wxTextCtrl(this, -1, content, wxDefaultPosition, wxDefaultSize,
                                    wxTE_READONLY | wxTE_MULTILINE | wxTE_RICH2 | wxHSCROLL);
            m_text->SetFont(m_font);

            sizer->Add(m_text, 1, wxGROW);

            SetSizer(sizer);
            sizer->Layout();
        }
        void SetText(const wxString &text)
        {
            m_text->SetValue(text);
            m_text->SetFont(m_font);
        }
    private:
        wxTextCtrl* m_text;
        wxFont m_font;
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
