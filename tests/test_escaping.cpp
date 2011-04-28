#include "common.h"

#include "escape.h"

TEST(Empty)
{
    CHECK_EQUAL(wxEmptyString, dbg_mi::EscapePath(wxEmptyString));
}

TEST(NoChange)
{
    CHECK_EQUAL(wxT("test/file.ext"), dbg_mi::EscapePath(wxT("test/file.ext")));
}

TEST(Spaces)
{
    CHECK_EQUAL(wxT("\"test/file name.ext\""), dbg_mi::EscapePath(wxT("test/file name.ext")));
}

TEST(IgnoreDoubleEscape)
{
    CHECK_EQUAL(wxT("\"test/file name.ext\""), dbg_mi::EscapePath(wxT("\"test/file name.ext\"")));
}

TEST(EscapeDoubleQuotes)
{
    CHECK_EQUAL(wxT("\"test\\\"file.ext\""), dbg_mi::EscapePath(wxT("test\"file.ext")));
}

TEST(ConvertDirectoryNoChange)
{
    wxString s(wxT("filename.exe"));
    dbg_mi::ConvertDirectory(s, wxEmptyString, false);
    CHECK_EQUAL(wxString(wxT("filename.exe")), s);
}

TEST(ConvertDirectoryNoPathEscape)
{
    wxString s(wxT("file name.exe"));
    dbg_mi::ConvertDirectory(s, wxEmptyString, false);
    CHECK_EQUAL(wxString(wxT("\"file name.exe\"")), s);
}

TEST(ConvertDirectorySimpleConcat)
{
    wxString s(wxT("filename.exe"));
    dbg_mi::ConvertDirectory(s, wxT("simple_path"), false);
    CHECK_EQUAL(wxT("simple_path/filename.exe"), s);
}

TEST(ConvertDirectoryConcatAndEscape)
{
    wxString s(wxT("fil ename.exe"));
    dbg_mi::ConvertDirectory(s, wxT("sim\"ple_path"), false);
    CHECK_EQUAL(wxT("\"sim\\\"ple_path/fil ename.exe\""), s);
}
