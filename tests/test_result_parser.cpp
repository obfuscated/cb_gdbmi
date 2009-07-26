#include <tr1/memory>
#include <UnitTest++.h>

#include "cmd_result_parser.h"
#include "cmd_result_tokens.h"

namespace dbg_mi
{
    extern wxString TrimmedSubString(wxString const &str, int start, int length);
}
TEST(TrimmedSubString)
{
     wxString str = dbg_mi::TrimmedSubString(_T("asd as    df"), 4, 3);

     CHECK(str == _T("as"));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<int N>
bool TestGetNextToken(wxString const &s, gdb_mi::Token &t)
{
    int pos = 0;
    for(int ii = 0; ii < N; ++ii)
    {
        if(!gdb_mi::GetNextToken(s, pos, t))
            return false;

        pos = t.end;
    }
    return true;
};

TEST(TestGetNextToken1)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<1>(_T("a = 5, b = 6"), t);

    CHECK(r && t.start == 0 && t.end == 1 && t.type == gdb_mi::Token::String);
}
TEST(TestGetNextToken2)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<2>(_T("a = 5, b = 6"), t);

    CHECK(r && t.start == 2 && t.end == 3 && t.type == gdb_mi::Token::Equal);
}
TEST(TestGetNextToken3)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<3>(_T("ab = 5, b = 6"), t);

    CHECK(r && t.start == 5 && t.end == 6 && t.type == gdb_mi::Token::String);
}
TEST(TestGetNextToken4)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<4>(_T("a = 5, b = 6"), t);

    CHECK(r && t.start == 5 && t.end == 6 && t.type == gdb_mi::Token::Comma);
}
TEST(TestGetNextToken5)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<5>(_T("a = 5, bdb=\"str\""), t);

    CHECK(r && t.start == 7 && t.end == 10 && t.type == gdb_mi::Token::String);
}
TEST(TestGetNextToken6)
{
    gdb_mi::Token t;
    wxString s = _T("a = 5, bdb=\"str\"");
    bool r = TestGetNextToken<7>(s, t);

    CHECK(r && t.start == 11 && t.end == 16 && t.type == gdb_mi::Token::String);
}
TEST(TestGetNextToken7)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<3>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t.start == 4 && t.end == 5 && t.type == gdb_mi::Token::ListStart);
}
TEST(TestGetNextToken8)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<4>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == gdb_mi::Token(5, 6, gdb_mi::Token::String));
}
TEST(TestGetNextToken9)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<5>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == gdb_mi::Token(6, 7, gdb_mi::Token::Comma));
}
TEST(TestGetNextToken10)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<6>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == gdb_mi::Token(7, 13, gdb_mi::Token::String));
}
TEST(TestGetNextToken11)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<9>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == gdb_mi::Token(16, 17, gdb_mi::Token::ListEnd));
}
TEST(TestGetNextToken12)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<10>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == gdb_mi::Token(17, 18, gdb_mi::Token::Comma));
}
TEST(TestGetNextToken13)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<13>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == gdb_mi::Token(22, 23, gdb_mi::Token::TupleStart));
}
TEST(TestGetNextToken14)
{
    gdb_mi::Token t;
    bool r = TestGetNextToken<21>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == gdb_mi::Token(39, 40, gdb_mi::Token::TupleEnd));
}

TEST(TestGetNextToken15)
{
    gdb_mi::Token t;
    wxString s = _T("a = \"-\\\"ast\\\"-\"");
    bool r = TestGetNextToken<3>(s, t);
    CHECK(r && t == gdb_mi::Token(4, 15, gdb_mi::Token::String));
    CHECK(r && t.ExtractString(s) == _T("\"-\\\"ast\\\"-\""));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestSimpleValue
{
    TestSimpleValue()
    {
        status = dbg_mi::ParseValue(_T("a = 5"), results);

        result = results.GetTupleValue(_T("a"));
    }

    dbg_mi::ResultValue results;
    dbg_mi::ResultValue const *result;
    bool status;
};

TEST_FIXTURE(TestSimpleValue, Status)
{
    CHECK(status);
}

TEST_FIXTURE(TestSimpleValue, ResultValue_SimpleValueType)
{
    CHECK(result && result->GetType() == dbg_mi::ResultValue::Simple);
}
TEST_FIXTURE(TestSimpleValue, ResultValue_SimpleName)
{
    CHECK(result && result->GetName() == _T("a"));
}

TEST_FIXTURE(TestSimpleValue, ResultValue_SimpleValue)
{
    CHECK(result && result->GetSimpleValue() == _T("5"));
}

TEST_FIXTURE(TestSimpleValue, TestSimple_DebugString)
{
    CHECK(results.MakeDebugString() == _T("{a=5}"));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestSimpleValue2
{
    TestSimpleValue2()
    {
        status = dbg_mi::ParseValue(_T("a = 5, b = 6"), result);
//        wprintf(L">%s<\n", result.MakeDebugString().utf8_str().data());

        v_a = result.GetTupleValue(_T("a"));
        v_b = result.GetTupleValue(_T("b"));
    }

    dbg_mi::ResultValue result;
    dbg_mi::ResultValue const *v_a;
    dbg_mi::ResultValue const *v_b;
    bool status;
};


TEST_FIXTURE(TestSimpleValue2, Status)
{
    CHECK(status);
}

TEST_FIXTURE(TestSimpleValue2, Count)
{
    CHECK(result.GetTupleSize() == 2);
}

TEST_FIXTURE(TestSimpleValue2, Values)
{
    CHECK(v_a);
    CHECK(v_b);
}

TEST_FIXTURE(TestSimpleValue2, ResultValue_SimpleValueType1)
{
    CHECK(v_a && v_a->GetType() == dbg_mi::ResultValue::Simple);
}
TEST_FIXTURE(TestSimpleValue2, ResultValue_SimpleValueType2)
{
    CHECK(v_b && v_b->GetType() == dbg_mi::ResultValue::Simple);
}

TEST_FIXTURE(TestSimpleValue2, ResultValue_SimpleValue1)
{
    CHECK(v_a && v_a->GetSimpleValue() == _T("5"));
}

TEST_FIXTURE(TestSimpleValue2, ResultValue_SimpleValue2)
{
    CHECK(v_b && v_b->GetSimpleValue() == _T("6"));
}

TEST_FIXTURE(TestSimpleValue2, TestSimple_DebugString2)
{
    CHECK(result.MakeDebugString() == _T("{a=5,b=6}"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestTupleValue
{
    TestTupleValue()
    {
        status = dbg_mi::ParseValue(_T("a = {b = 5, c = 6, d = {e = 7, f = 8}}, b1 = 51"), result);
//        wprintf(L">%s<\n", result.MakeDebugString().utf8_str().data());
        v_a = result.GetTupleValue(_T("a"));
    }

    dbg_mi::ResultValue result;
    dbg_mi::ResultValue const *v_a;
    bool status;
};

TEST_FIXTURE(TestTupleValue, TupleStatus)
{
    CHECK(status);
    CHECK(v_a);
}

TEST_FIXTURE(TestTupleValue, TupleValue1)
{
    dbg_mi::ResultValue const *b = NULL;

    if(v_a)
        b = v_a->GetTupleValue(_T("b"));
    CHECK(b && b->GetType() == dbg_mi::ResultValue::Simple && b->GetSimpleValue() == _T("5"));
}

TEST_FIXTURE(TestTupleValue, TupleValue2)
{
    dbg_mi::ResultValue const *c = NULL;

    if(v_a)
        c = v_a->GetTupleValue(_T("c"));
    CHECK(c && c->GetType() == dbg_mi::ResultValue::Simple && c->GetSimpleValue() == _T("6"));
}

TEST_FIXTURE(TestTupleValue, TupleValue3)
{
    dbg_mi::ResultValue const *d = NULL;

    if(v_a)
        d = v_a->GetTupleValue(_T("d"));
    CHECK(d && d->GetType() == dbg_mi::ResultValue::Tuple);
}

TEST_FIXTURE(TestTupleValue, TupleValue4)
{
    dbg_mi::ResultValue const *d = NULL;
    dbg_mi::ResultValue const *e = NULL;
    dbg_mi::ResultValue const *f = NULL;

    if(v_a)
    {
        d = v_a->GetTupleValue(_T("d"));
        if(d)
        {
            e = d->GetTupleValue(_T("e"));
            f = d->GetTupleValue(_T("f"));
        }
    }

    CHECK(d && d->GetType() == dbg_mi::ResultValue::Tuple);
    CHECK(e && e->GetType() == dbg_mi::ResultValue::Simple && e->GetSimpleValue() == _T("7"));
    CHECK(f && f->GetType() == dbg_mi::ResultValue::Simple && f->GetSimpleValue() == _T("8"));
}

TEST_FIXTURE(TestSimpleValue2, Tuple_DebugString)
{
    bool status = result.MakeDebugString() == _T("{a={b=5,c=6,d={e=7,f=8}},b1=51}");
    CHECK_EQUAL(status, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestListValue
{
    TestListValue()
    {
        status = dbg_mi::ParseValue(_T("a = [5,6,7]"), result);

//        printf(">%s<\n", result.MakeDebugString().utf8_str().data());
    }

    dbg_mi::ResultValue result;
    bool status;
};
//
//TEST_FIXTURE(TestListValue, ListStatus)
//{
//    CHECK(status);
//    CHECK(result.GetTupleSize() == 3);
//}
//
//TEST_FIXTURE(TestListValue, ListValue1)
//{
//    dbg_mi::ResultValue const *v = result.GetTupleValueByIndex(0);
//    CHECK(v && v->GetSimpleValue() == _T("5"));
//}
//
//TEST_FIXTURE(TestListValue, ListValue2)
//{
//    dbg_mi::ResultValue const *v = result.GetTupleValueByIndex(1);
//    CHECK(v && v->GetSimpleValue() == _T("6"));
//}
//
//TEST_FIXTURE(TestListValue, ListValue3)
//{
//    dbg_mi::ResultValue const *v = result.GetTupleValueByIndex(2);
//    CHECK(v && v->GetSimpleValue() == _T("7"));
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(ResultParse_TestType)
{
    dbg_mi::ResultParser parser;
    CHECK(parser.Parse(_T("done,a = 5, b = 6"), dbg_mi::ResultParser::Result));

    CHECK_EQUAL(parser.GetResultType(), dbg_mi::ResultParser::Result);
}

TEST(ResultParse_TestClass)
{
    dbg_mi::ResultParser parser;
    CHECK(parser.Parse(_T("done,a = 5, b = 6"), dbg_mi::ResultParser::Result));

    CHECK(parser.GetResultClass() == dbg_mi::ResultParser::ClassDone);
}
TEST(ResultParse_NoClassComma)
{
    dbg_mi::ResultParser parser;
    bool r = parser.Parse(_T("done a = 5, b = 6"), dbg_mi::ResultParser::Result);

    CHECK(!r);
}

TEST(ResultParse_TestValues)
{
    dbg_mi::ResultParser parser;
    parser.Parse(_T("done,a = 5, b = 6"), dbg_mi::ResultParser::Result);

    dbg_mi::ResultValue::Container const &v = parser.GetResultValues();

    CHECK_EQUAL(v.size(), 2u);
    CHECK(v[0]->raw_value == _T("a = 5"));
    CHECK(v[1]->raw_value == _T(" b = 6"));
}
