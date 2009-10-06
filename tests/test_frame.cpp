#include <UnitTest++.h>

#include "cmd_result_parser.h"
#include "frame.h"

wxString const c_frame_output_valid(wxT("thread-id=\"0\",frame={addr=\"0x00000000004019eb\",func=\"main\",")
                                    wxT("args=[{name=\"argc\",value=\"1\"},{name=\"argv\",")
                                    wxT("value=\"0x7fffd655ae38 \\\"\\\\362\\\\300U\\\\326\\\\377\\\\177\\\"\"}],")
                                    wxT("file=\"/home/user/projects/tests/main.cpp\",")
                                    wxT("fullname=\"/home/user/projects/tests/main.cpp\",")
                                    wxT("line=\"187\"}"));

// output of frame for notification with file/line info
wxString const c_frame_output_no_source(wxT("reason=\"end-stepping-range\",thread-id=\"0\",")
                                        wxT("frame={addr=\"0x00007fd9c8e68486\",func=\"__libc_start_main\",")
                                        wxT("args=[],from=\"/lib/libc.so.6\"}"));

TEST(FrameCtor)
{
    dbg_mi::Frame frame;

    CHECK(frame.GetLine() == -1 && frame.GetFilename() == wxEmptyString && frame.GetFullFilename() == wxEmptyString);
}

struct FrameStoppedFixture
{
    FrameStoppedFixture(wxString const &output) :
        status(false)
    {
        dbg_mi::ResultValue output_value;
        if(dbg_mi::ParseValue(output, output_value, 0))
        {
            status = frame.Parse(output_value);
            if(!status)
                printf(">>> %s\n", output_value.MakeDebugString().utf8_str().data());
        }
    }
    bool status;
    dbg_mi::Frame frame;
};
TEST(FrameStoppedFixture_Valid_HasValidSource)
{
    FrameStoppedFixture f(c_frame_output_valid);
    CHECK(f.frame.HasValidSource());
}

TEST(FrameStoppedFixture_NoSource_Status)
{
    FrameStoppedFixture f(c_frame_output_no_source);
    CHECK(f.status);
}

TEST(FrameStoppedFixture_NoSource_HasValidSource)
{
    FrameStoppedFixture f(c_frame_output_no_source);
    CHECK(!f.frame.HasValidSource());
}

dbg_mi::StoppedReason TestReason(wxString const &gdb_output)
{
    dbg_mi::ResultValue value;
    if(!dbg_mi::ParseValue(gdb_output, value, 0))
        return dbg_mi::StoppedReason::Unknown;
    return dbg_mi::StoppedReason::Parse(value);
}

TEST(StoppedReasonParse_BreakpointHit)
{
    CHECK(TestReason(wxT("reason=\"breakpoint-hit\"")) == dbg_mi::StoppedReason::BreakpointHit);
}

TEST(StoppedReasonParse_ExitedNormally)
{
    CHECK(TestReason(wxT("reason=\"exited-normally\"")) == dbg_mi::StoppedReason::ExitedNormally);
}
