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
                                        wxT("frame={addr=\"0xc8e68486\",func=\"__libc_start_main\",")
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
            status = frame.ParseOutput(output_value);
            if(!status)
                printf(">>> %s\n", output_value.MakeDebugString().utf8_str().data());
        }
    }
    bool status;
    dbg_mi::Frame frame;
};

TEST(FrameStoppedFixture_Valid_GetFunction)
{
    FrameStoppedFixture f(c_frame_output_valid);
    CHECK(f.frame.GetFunction() == wxT("main"));
}

TEST(FrameStoppedFixture_Valid_GetAddress)
{
    FrameStoppedFixture f(c_frame_output_valid);
    CHECK_EQUAL(0x00000000004019ebul, f.frame.GetAddress());
}

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

TEST(FrameStoppedFixture_NoSource_From)
{
    FrameStoppedFixture f(c_frame_output_no_source);
    CHECK(f.frame.GetFrom() == wxT("/lib/libc.so.6"));
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

TEST(StoppedReasonParse_Exited)
{
    CHECK(TestReason(wxT("reason=\"exited\"")) == dbg_mi::StoppedReason::Exited);
}

TEST(StoppedReasonParse_ExitedNormally)
{
    CHECK(TestReason(wxT("reason=\"exited-normally\"")) == dbg_mi::StoppedReason::ExitedNormally);
}

TEST(StoppedReasonParse_ExitedSignalled)
{
    CHECK(TestReason(wxT("reason=\"exited-signalled\"")) == dbg_mi::StoppedReason::ExitedSignalled);
}

TEST(StoppedReasonParse_SignalReceived)
{
    CHECK(TestReason(wxT("reason=\"signal-received\"")) == dbg_mi::StoppedReason::SignalReceived);
}



wxString const c_stack_args_output(
                wxT("stack-args=[")
                    wxT("frame={level=\"0\",args=[]},")
                    wxT("frame={level=\"1\",args=[")
                        wxT("{name=\"argc\",value=\"1\"},")
                        wxT("{name=\"argv\",value=\"0x7fffe8c04558 \\\"test.t\\\"\"}")
                    wxT("]}")
                wxT("]"));


struct FrameArgsFixture
{
    FrameArgsFixture()
    {
        status = dbg_mi::ParseValue(c_stack_args_output, r);
        if(status)
            status = args.Attach(r);
    }

    dbg_mi::ResultValue r;
    dbg_mi::FrameArguments args;
    bool status;
};

TEST_FIXTURE(FrameArgsFixture, Status)
{
    CHECK(status);
}

TEST_FIXTURE(FrameArgsFixture, Size)
{
    CHECK_EQUAL(2, args.GetCount());
}

TEST_FIXTURE(FrameArgsFixture, Frame0)
{
    wxString a;
    CHECK(args.GetFrame(0, a));
    CHECK(a == wxEmptyString);
}

TEST_FIXTURE(FrameArgsFixture, Frame1)
{
    wxString a;
    CHECK(args.GetFrame(1, a));
    CHECK(a == wxT("argc=1, argv=0x7fffe8c04558 \"test.t\""));
}

