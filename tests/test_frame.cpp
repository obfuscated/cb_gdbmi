#include <UnitTest++.h>

#include "cmd_result_parser.h"
#include "frame.h"

TEST(FrameCtor)
{
    dbg_mi::Frame frame;

    CHECK(frame.GetLine() == -1 && frame.GetFilename() == wxEmptyString && frame.GetFullFilename() == wxEmptyString);
}

struct FrameStoppedFixture
{
    FrameStoppedFixture() :
        status(false)
    {
        dbg_mi::ResultValue frame_value;
        if(dbg_mi::ParseValue(wxT("reason=\"stopped\""), frame_value, 0))
            status = frame.Parse(frame_value);
    }
    bool status;
    dbg_mi::Frame frame;
};


//TEST_FIXTURE(FrameStoppedFixture, Status)
//{
//    CHECK(status);
//}
#warning "should implement"

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
