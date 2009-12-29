#include "frame.h"

#include "cmd_result_parser.h"

namespace dbg_mi
{

bool Frame::ParseOutput(ResultValue const &output_value)
{
    if(output_value.GetType() != ResultValue::Tuple)
        return false;

    dbg_mi::ResultValue const *frame_value = output_value.GetTupleValue(wxT("frame"));
    if(!frame_value)
        return false;
    return ParseFrame(*frame_value);
}

bool Frame::ParseFrame(ResultValue const &frame_value)
{
    ResultValue const *function = frame_value.GetTupleValue(wxT("func"));
    if(function)
        m_function = function->GetSimpleValue();
    ResultValue const *address = frame_value.GetTupleValue(wxT("addr"));
    if(address)
    {
        wxString const &str = address->GetSimpleValue();
        if(!str.ToULong(&m_address, 16))
            return false;
    }

    ResultValue const *from = frame_value.GetTupleValue(wxT("from"));
    if(from)
        m_from = from->GetSimpleValue();

    ResultValue const *line = frame_value.GetTupleValue(_T("line"));
    ResultValue const *filename = frame_value.GetTupleValue(_T("file"));
    ResultValue const *full_filename = frame_value.GetTupleValue(_T("fullname"));

    if(!line && !filename && !full_filename)
    {
        m_has_valid_source = false;
        return true;
    }
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
    m_has_valid_source = true;

    return true;
}

FrameArguments::FrameArguments() :
    m_stack_args(NULL)
{
}

bool FrameArguments::Attach(ResultValue const &output)
{
    if(output.GetType() != ResultValue::Tuple)
        return false;

    m_stack_args = output.GetTupleValue(wxT("stack-args"));
    return m_stack_args;
}

int FrameArguments::GetCount() const
{
    return m_stack_args->GetTupleSize();
}

bool FrameArguments::GetFrame(int index, wxString &args) const
{
    ResultValue const *frame = m_stack_args->GetTupleValueByIndex(index);
    if(!frame || frame->GetName() != wxT("frame"))
        return false;

    return ParseFrame(*frame, args);
}

bool FrameArguments::ParseFrame(ResultValue const &frame_value, wxString &args)
{
    args = wxEmptyString;

    ResultValue const *args_tuple = frame_value.GetTupleValue(wxT("args"));
    if(!args_tuple || args_tuple->GetType() != ResultValue::Array)
        return false;

    for(int ii = 0; ii < args_tuple->GetTupleSize(); ++ii)
    {
        ResultValue const *arg = args_tuple->GetTupleValueByIndex(ii);
        assert(arg);

        ResultValue const *name = arg->GetTupleValue(wxT("name"));
        ResultValue const *value = arg->GetTupleValue(wxT("value"));

        if(name && name->GetType() == ResultValue::Simple
           && value && value->GetType() == ResultValue::Simple)
        {
            if(!args.empty())
                args += wxT(", ");
            args += name->GetSimpleValue() + wxT("=") + value->GetSimpleValue();
        }
        else
            return false;
    }

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
    else if(str == wxT("exited-signalled"))
        return ExitedSignalled;
    else if(str == wxT("exited"))
        return Exited;
    else if(str == wxT("exited-normally"))
        return ExitedNormally;
    else if(str == wxT("signal-received"))
        return SignalReceived;
    else
        return Unknown;
}

} // namespace dbg_mi
