#ifndef _DEBUGGER_GDB_MI_UPDATED_VARIABLE_H_
#define _DEBUGGER_GDB_MI_UPDATED_VARIABLE_H_

#include <wx/string.h>

namespace dbg_mi
{

class ResultValue;

class UpdatedVariable
{
public:
    enum InScope
    {
        InScope_Yes = 0,
        InScope_No,
        InScope_Invalid
    };
public:
    UpdatedVariable() :
        m_inscope(InScope_Invalid),
        m_new_num_children(-1),
        m_type_changed(false),
        m_has_value(false)
    {
    }

    InScope GetInScope() const { return m_inscope; }
    wxString const & GetName() const { return m_name; }
    wxString const & GetValue() const { return m_value; }
    wxString const & GetNewType() const { return m_new_type; }
    bool TypeChanged() const { return m_type_changed; }
    bool HasValue() const { return m_has_value; }
    int GetNewNumberOfChildren() const { return m_new_num_children; }

    bool Parse(ResultValue const &output);

private:
    wxString m_name;
    wxString m_value;
    wxString m_new_type;
    InScope m_inscope;
    int m_new_num_children;
    bool m_type_changed;
    bool m_has_value;
};



} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_UPDATED_VARIABLE_H_
