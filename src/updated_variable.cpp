#include "updated_variable.h"
#include "cmd_result_parser.h"

namespace dbg_mi
{

bool UpdatedVariable::Parse(ResultValue const &output)
{
    m_new_num_children = -1;
    wxString str;
    if(Lookup(output, wxT("in_scope"), str))
    {
        if(str == wxT("true"))
            m_inscope = InScope_Yes;
        else if(str == wxT("false"))
            m_inscope = InScope_No;
        else if(str == wxT("invalid"))
            m_inscope = InScope_Invalid;
    }
    else
        return false;

    if(!Lookup(output, wxT("name"), m_name))
        return false;

    if(!Lookup(output, wxT("type_changed"), m_type_changed))
        return false;

    if(Lookup(output, wxT("value"), m_value))
        m_has_value = true;

    if(m_type_changed)
    {
        if(!Lookup(output, wxT("new_type"), m_new_type))
            return false;
        if(!Lookup(output, wxT("new_num_children"), m_new_num_children))
            return false;
    }
    return true;
}

} // namespace dbg_mi
