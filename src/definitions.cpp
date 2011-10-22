#include "definitions.h"

namespace dbg_mi
{
void Breakpoint::SetEnabled(bool flag)
{
}

wxString Breakpoint::GetLocation() const
{
    return m_filename;
}

int Breakpoint::GetLine() const
{
    return m_line;
}

wxString Breakpoint::GetLineString() const
{
    return wxString::Format(wxT("%d"), m_line);
}

wxString Breakpoint::GetType() const
{
    return _("Code");
}

wxString Breakpoint::GetInfo() const
{
    return wxEmptyString;
}

bool Breakpoint::IsEnabled() const
{
    return m_enabled;
}

bool Breakpoint::IsVisibleInEditor() const
{
    return true;
}

bool Breakpoint::IsTemporary() const
{
    return m_temporary;
}

Watch::Pointer FindWatch(wxString const &expression, WatchesContainer &watches)
{
    for(WatchesContainer::iterator it = watches.begin(); it != watches.end(); ++it)
    {
        if(expression.StartsWith(it->get()->GetID()))
        {
            if(expression.length() == it->get()->GetID().length())
                return *it;
            else
            {
                Watch::Pointer curr = *it;
                while(curr)
                {
                    Watch::Pointer temp = curr;
                    curr = Watch::Pointer();

                    for(int child = 0; child < temp->GetChildCount(); ++child)
                    {
                        Watch::Pointer p = cb::static_pointer_cast<Watch>(temp->GetChild(child));
                        if(expression.StartsWith(p->GetID()))
                        {
                            if(expression.length() == p->GetID().length())
                                return p;
                            else
                            {
                                curr = p;
                                break;
                            }
                        }
                    }
                }
                if(curr)
                    return curr;
            }
        }
    }
    return Watch::Pointer();
}

} // namespace dbg_mi
