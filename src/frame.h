#ifndef _DEBUGGER_GDB_MI_FRAME_H_
#define _DEBUGGER_GDB_MI_FRAME_H_

#include <wx/string.h>

namespace dbg_mi
{

class ResultValue;

class Frame
{
public:

    bool Parse(ResultValue const &frame_tuple);

    int GetLine() const { return m_line; }
    wxString const & GetFilename() const { return m_filename; }
    wxString const & GetFullFilename() const { return m_full_filename; }
private:
    wxString m_filename;
    wxString m_full_filename;
    int m_line;
};

} // namespace dbg_mi


#endif // _DEBUGGER_GDB_MI_FRAME_H_
