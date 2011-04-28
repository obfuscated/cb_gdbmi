#include "escape.h"

namespace dbg_mi
{

wxString EscapePath(wxString const &path)
{
    if (path.empty())
        return path;

    bool escape = false, escapeDoubleQuotes = false;
    for (size_t ii = 0; ii < path.length(); ++ii)
    {
        switch (path[ii])
        {
        case wxT(' '):
            escape = true;
            break;
        case wxT('"'):
            if (ii > 0 && ii < path.length() - 1)
                escapeDoubleQuotes = true;
            break;
        }
    }
    if (path[0] == wxT('"') && path[path.length()-1] == wxT('"'))
        escape = false;
    if (!escape && !escapeDoubleQuotes)
        return path;

    wxString result;
    if (path[0] == wxT('"') && path[path.length()-1] == wxT('"'))
        result = path.substr(1, path.length() - 2);
    else
        result = path;
    result.Replace(wxT("\""), wxT("\\\""));
    return wxT('"') + result + wxT('"');
}

void ConvertDirectory(wxString& str, wxString base, bool relative)
{
    if (!base.empty())
        str = base + wxT("/") + str;
    str = EscapePath(str);
}

} // namespace dbg_mi
