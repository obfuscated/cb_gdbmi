#include "definitions.h"

namespace dbg_mi
{

Watch* FindWatch(wxString const &expression, WatchesContainer &watches)
{
    for(WatchesContainer::iterator it = watches.begin(); it != watches.end(); ++it)
    {
        if(expression.StartsWith(it->get()->GetID()))
        {
            if(expression.length() == it->get()->GetID().length())
                return it->get();
            else
            {
                Watch *curr = it->get();
                while(curr)
                {
                    Watch *temp = curr;
                    curr = NULL;

                    for(int child = 0; child < temp->GetChildCount(); ++child)
                    {
                        Watch *p = static_cast<Watch*>(temp->GetChild(child));
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
    return NULL;
}

} // namespace dbg_mi
