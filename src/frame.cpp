#include "frame.h"

#include "cmd_result_parser.h"

namespace dbg_mi
{

bool Frame::Parse(ResultValue const &frame_tuple)
{
    if(frame_tuple.GetType() != ResultValue::Tuple)
        return false;

    ResultValue const *line = frame_tuple.GetTupleValue(_T("line"));
    ResultValue const *filename = frame_tuple.GetTupleValue(_T("file"));
    ResultValue const *full_filename = frame_tuple.GetTupleValue(_T("fullname"));
    if((!line || line->GetType() != ResultValue::Simple)
       || (!filename || filename->GetType() != ResultValue::Simple)
       || (!full_filename || full_filename->GetType() != ResultValue::Simple))
    {
        return false;
    }

    m_filename = filename->GetSimpleValue();
    m_full_filename = full_filename->GetSimpleValue();
    long long_line;
    if(!line->GetSimpleValue().ToLong(&long_line))
        return false;

    m_line = long_line;

    return true;
}

StoppedReason StoppedReason::Parse(ResultValue const &value)
{
    ResultValue const *reason = value.GetTupleValue(wxT("reason"));
    if(!reason)
        return Unknown;
    wxString const &str = reason->GetSimpleValue();
    if(str == wxT("breakpoint-hit"))
        return BreakpointHit;
    else if(str == wxT("exited-normally"))
        return ExitedNormally;
    else
        return Unknown;
}

} // namespace dbg_mi
