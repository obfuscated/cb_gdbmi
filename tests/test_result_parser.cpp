#include <tr1/memory>
#include <UnitTest++.h>

#include "cmd_result_parser.h"
#include "cmd_result_tokens.h"

namespace dbg_mi
{
extern bool StripEnclosingQuotes(wxString &str);
}
TEST(StripEnclosingQuotes1)
{
     wxString str(_T("\"test \\\"mega\\\"\""));
     bool status = dbg_mi::StripEnclosingQuotes(str);

     CHECK(status && str == _T("test \"mega\""));
}
TEST(StripEnclosingQuotes2)
{
     wxString str(_T("\"test"));
     bool status = dbg_mi::StripEnclosingQuotes(str);

     CHECK(!status);
}
TEST(StripEnclosingQuotes3)
{
     wxString str(_T("test\""));
     bool status = dbg_mi::StripEnclosingQuotes(str);

     CHECK(!status);
}
TEST(StripEnclosingQuotes4)
{
     wxString str(_T("test\\\"mega\\\""));
     bool status = dbg_mi::StripEnclosingQuotes(str);

     CHECK(status && str == _T("test\"mega\""));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<int N>
bool TestGetNextToken(wxString const &s, dbg_mi::Token &t)
{
    int pos = 0;
    for(int ii = 0; ii < N; ++ii)
    {
        if(!dbg_mi::GetNextToken(s, pos, t))
            return false;

        pos = t.end;
    }
    return true;
};

TEST(TestGetNextToken1)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<1>(_T("a = 5, b = 6"), t);

    CHECK(r && t.start == 0 && t.end == 1 && t.type == dbg_mi::Token::String);
}
TEST(TestGetNextToken2)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<2>(_T("a = 5, b = 6"), t);

    CHECK(r && t.start == 2 && t.end == 3 && t.type == dbg_mi::Token::Equal);
}
TEST(TestGetNextToken3)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<3>(_T("ab = 5, b = 6"), t);

    CHECK(r && t.start == 5 && t.end == 6 && t.type == dbg_mi::Token::String);
}
TEST(TestGetNextToken4)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<4>(_T("a = 5, b = 6"), t);

    CHECK(r && t.start == 5 && t.end == 6 && t.type == dbg_mi::Token::Comma);
}
TEST(TestGetNextToken5)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<5>(_T("a = 5, bdb=\"str\""), t);

    CHECK(r && t.start == 7 && t.end == 10 && t.type == dbg_mi::Token::String);
}
TEST(TestGetNextToken6)
{
    dbg_mi::Token t;
    wxString s = _T("a = 5, bdb=\"str\"");
    bool r = TestGetNextToken<7>(s, t);

    CHECK(r && t.start == 11 && t.end == 16 && t.type == dbg_mi::Token::String);
}
TEST(TestGetNextToken7)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<3>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t.start == 4 && t.end == 5 && t.type == dbg_mi::Token::ListStart);
}
TEST(TestGetNextToken8)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<4>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(5, 6, dbg_mi::Token::String));
}
TEST(TestGetNextToken9)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<5>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(6, 7, dbg_mi::Token::Comma));
}
TEST(TestGetNextToken10)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<6>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(7, 13, dbg_mi::Token::String));
}
TEST(TestGetNextToken11)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<9>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(16, 17, dbg_mi::Token::ListEnd));
}
TEST(TestGetNextToken12)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<10>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(17, 18, dbg_mi::Token::Comma));
}
TEST(TestGetNextToken13)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<13>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(22, 23, dbg_mi::Token::TupleStart));
}
TEST(TestGetNextToken14)
{
    dbg_mi::Token t;
    bool r = TestGetNextToken<21>(_T("a = [5,\"sert\", 6],bdb={a = \"str\", b = 5}"), t);
    CHECK(r && t == dbg_mi::Token(39, 40, dbg_mi::Token::TupleEnd));
}

TEST(TestGetNextToken15)
{
    dbg_mi::Token t;
    wxString s = _T("a = \"-\\\"ast\\\"-\"");
    bool r = TestGetNextToken<3>(s, t);
    CHECK(r && t == dbg_mi::Token(4, 15, dbg_mi::Token::String));
    CHECK(r && t.ExtractString(s) == _T("\"-\\\"ast\\\"-\""));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(ResultValueMakeDebugString_Simple)
{
    dbg_mi::ResultValue r(_T("name"), dbg_mi::ResultValue::Simple);
    r.SetSimpleValue(_T("value"));

    CHECK(r.MakeDebugString() == _T("name=value"));
}
TEST(ResultValueMakeDebugString_Tuple)
{
    dbg_mi::ResultValue r(_T("name"), dbg_mi::ResultValue::Tuple);

    dbg_mi::ResultValue *a = new dbg_mi::ResultValue(_T("name1"), dbg_mi::ResultValue::Simple);
    a->SetSimpleValue(_T("value1"));

    dbg_mi::ResultValue *b = new dbg_mi::ResultValue(_T("name2"), dbg_mi::ResultValue::Simple);
    b->SetSimpleValue(_T("value2"));

    r.SetTupleValue(a);
    r.SetTupleValue(b);

    wxString s = r.MakeDebugString();
    CHECK(s == _T("name={name1=value1,name2=value2}"));
}
TEST(ResultValueMakeDebugString3)
{
    dbg_mi::ResultValue r(_T("name"), dbg_mi::ResultValue::Array);

    dbg_mi::ResultValue *a = new dbg_mi::ResultValue(_T(""), dbg_mi::ResultValue::Simple);
    a->SetSimpleValue(_T("1"));

    dbg_mi::ResultValue *b = new dbg_mi::ResultValue(_T(""), dbg_mi::ResultValue::Simple);
    b->SetSimpleValue(_T("2"));

    dbg_mi::ResultValue *c = new dbg_mi::ResultValue(_T(""), dbg_mi::ResultValue::Simple);
    c->SetSimpleValue(_T("3"));

    r.SetTupleValue(a);
    r.SetTupleValue(b);
    r.SetTupleValue(c);

    wxString s = r.MakeDebugString();
    CHECK(s == _T("name=[1,2,3]"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
struct TestTuple1
{
    TestTuple1()
    {
        status = dbg_mi::ParseValue(_T("a = {b = 5, c = 6}"), result);
        v_a = result.GetTupleValue(_T("a"));
    }

    dbg_mi::ResultValue result;
    dbg_mi::ResultValue const *v_a;
    bool status;
};

TEST_FIXTURE(TestTuple1, Tuple_Status)
{
    CHECK(status);
}

TEST_FIXTURE(TestTuple1, Tuple_DebugString)
{
    wxString s = result.MakeDebugString();
    CHECK(s == _T("{a={b=5,c=6}}"));
}

TEST_FIXTURE(TestTuple1, Tuple_ValueA_Type)
{
    CHECK(v_a && v_a->GetType() == dbg_mi::ResultValue::Tuple);
}

TEST_FIXTURE(TestTuple1, Tuple_ValueA_Value)
{
    dbg_mi::ResultValue const *b = v_a ? v_a->GetTupleValue(_T("b")) : NULL;
    dbg_mi::ResultValue const *c = v_a ? v_a->GetTupleValue(_T("c")) : NULL;

    CHECK(b && b->GetType() == dbg_mi::ResultValue::Simple && b->GetSimpleValue() == _T("5"));
    CHECK(c && c->GetType() == dbg_mi::ResultValue::Simple && c->GetSimpleValue() == _T("6"));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestTuple2
{
    TestTuple2()
    {
        status = dbg_mi::ParseValue(_T("a = {b = 5, c = {c1= 1,c2 =2}}, d=6"), result);
        v_a = result.GetTupleValue(_T("a"));
    }

    dbg_mi::ResultValue result;
    dbg_mi::ResultValue const *v_a;
    dbg_mi::ResultValue const *v_d;
    bool status;
};


TEST_FIXTURE(TestTuple2, Tuple_Status)
{
    CHECK(status);
}

TEST_FIXTURE(TestTuple2, Tuple_DebugString)
{
    wxString s = result.MakeDebugString();
    CHECK(s == _T("{a={b=5,c={c1=1,c2=2}},d=6}"));
}

TEST_FIXTURE(TestTuple2, Tuple_ValueB)
{
    dbg_mi::ResultValue const *b = v_a ? v_a->GetTupleValue(_T("b")) : NULL;

    CHECK(b && b->GetType() == dbg_mi::ResultValue::Simple && b->GetSimpleValue() == _T("5"));
}
TEST_FIXTURE(TestTuple2, Tuple_ValueC1)
{
    dbg_mi::ResultValue const *c = v_a ? v_a->GetTupleValue(_T("c")) : NULL;

    CHECK(c && c->GetType() == dbg_mi::ResultValue::Tuple);
}
TEST_FIXTURE(TestTuple2, Tuple_ValueC2)
{
    dbg_mi::ResultValue const *c = v_a ? v_a->GetTupleValue(_T("c")) : NULL;
    dbg_mi::ResultValue const *c1 = c ? c->GetTupleValue(_T("c1")) : NULL;
    dbg_mi::ResultValue const *c2 = c ? c->GetTupleValue(_T("c2")) : NULL;

    CHECK(c1 && c1->GetType() == dbg_mi::ResultValue::Simple && c1->GetSimpleValue() == _T("1"));
    CHECK(c2 && c2->GetType() == dbg_mi::ResultValue::Simple && c2->GetSimpleValue() == _T("2"));
}

TEST_FIXTURE(TestTuple2, Tuple_ValueD)
{
    dbg_mi::ResultValue const *d = result.GetTupleValue(_T("d"));

    CHECK(d && d->GetType() == dbg_mi::ResultValue::Simple && d->GetSimpleValue() == _T("6"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestList
{
    TestList()
    {
        status = dbg_mi::ParseValue(_T("a = [5,6,7]"), result);

        a = result.GetTupleValue(_T("a"));
    }

    dbg_mi::ResultValue result;
    dbg_mi::ResultValue const *a;
    bool status;
};

TEST_FIXTURE(TestList, Status)
{
    CHECK(status);
    CHECK(a && a->GetTupleSize() == 3);
}

TEST_FIXTURE(TestList, DebugString)
{
    wxString s = result.MakeDebugString();
    CHECK_EQUAL(wxT("{a=[5,6,7]}"), s);
}

TEST_FIXTURE(TestList, ListValue1)
{
    dbg_mi::ResultValue const *v = a ? a->GetTupleValueByIndex(0) : NULL;
    CHECK(v && v->GetSimpleValue() == _T("5"));
}

TEST_FIXTURE(TestList, ListValue2)
{
    dbg_mi::ResultValue const *v = a ? a->GetTupleValueByIndex(1) : NULL;
    CHECK(v && v->GetSimpleValue() == _T("6"));
}

TEST_FIXTURE(TestList, ListValue3)
{
    dbg_mi::ResultValue const *v = a ? a->GetTupleValueByIndex(2) : NULL;
    CHECK(v && v->GetSimpleValue() == _T("7"));
}

struct TestListWithTuples
{
    TestListWithTuples()
    {
        status = dbg_mi::ParseValue(_T("a = [{b=5},{c=6},{d=7}]"), result);

        a = result.GetTupleValue(_T("a"));
    }

    dbg_mi::ResultValue result;
    dbg_mi::ResultValue const *a;
    bool status;
};

TEST_FIXTURE(TestListWithTuples, Status)
{
    CHECK(status);
    CHECK(a && a->GetTupleSize() == 3);
}

TEST_FIXTURE(TestListWithTuples, DebugString)
{
    wxString s = result.MakeDebugString();
    CHECK(s == _T("{a=[{b=5},{c=6},{d=7}]}"));
}

TEST_FIXTURE(TestListWithTuples, ListValue1)
{
    dbg_mi::ResultValue const *v = a ? a->GetTupleValueByIndex(0) : NULL;
    dbg_mi::ResultValue const *b = v ? v->GetTupleValue(_T("b")) : NULL;

    CHECK(b && b->GetSimpleValue() == _T("5"));
}

TEST_FIXTURE(TestListWithTuples, ListValue2)
{
    dbg_mi::ResultValue const *v = a ? a->GetTupleValueByIndex(1) : NULL;
    dbg_mi::ResultValue const *c = v ? v->GetTupleValue(_T("c")) : NULL;

    CHECK(c && c->GetSimpleValue() == _T("6"));
}
TEST_FIXTURE(TestListWithTuples, ListValue3)
{
    dbg_mi::ResultValue const *v = a ? a->GetTupleValueByIndex(2) : NULL;
    dbg_mi::ResultValue const *d = v ? v->GetTupleValue(_T("d")) : NULL;

    CHECK(d && d->GetSimpleValue() == _T("7"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestListEmpty
{
    TestListEmpty()
    {
        status = dbg_mi::ParseValue(_T("a = []"), result);

        a = result.GetTupleValue(_T("a"));
    }

    dbg_mi::ResultValue result;
    dbg_mi::ResultValue const *a;
    bool status;
};

TEST_FIXTURE(TestListEmpty, Status)
{
    CHECK(status);
}

TEST_FIXTURE(TestListEmpty, DebugString)
{
    CHECK(result.MakeDebugString() == wxT("{a=[]}"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestListDuplicate
{
    TestListDuplicate()
    {
        status = dbg_mi::ParseValue(wxT("a=[a={b=5},a={b=6}]"), result);
        a = result.GetTupleValue(wxT("a"));
    }
    dbg_mi::ResultValue result;
    dbg_mi::ResultValue const *a;
    bool status;
};

TEST_FIXTURE(TestListDuplicate, Status)
{
    CHECK(status);
}

TEST_FIXTURE(TestListDuplicate, A)
{
    CHECK(a);
}

TEST_FIXTURE(TestListDuplicate, Size)
{
    CHECK(a && a->GetTupleSize() == 2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestTupleDuplicate
{
    TestTupleDuplicate()
    {
        status = dbg_mi::ParseValue(wxT("a={a={b=5},a={b=6}}"), result);
        a = result.GetTupleValue(wxT("a"));
    }
    dbg_mi::ResultValue result;
    dbg_mi::ResultValue const *a;
    bool status;
};

TEST_FIXTURE(TestTupleDuplicate, Status)
{
    CHECK(status);
}

TEST_FIXTURE(TestTupleDuplicate, A)
{
    CHECK(a);
}

TEST_FIXTURE(TestTupleDuplicate, Size)
{
    CHECK(a && a->GetTupleSize() == 2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(ParserFail1)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a"), r);
    CHECK(!status);
}
TEST(ParserFail2)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a,"), r);
    CHECK(!status);
}
TEST(ParserFail3_1)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = 5,"), r);
    CHECK(!status);
}
TEST(ParserFail3_2)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = 5,b="), r);
    CHECK(!status);
}
TEST(ParserFail3_3)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = {"), r);
    CHECK(!status);
}
TEST(ParserFail3_4)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = ["), r);
    CHECK(!status);
}
TEST(ParserFail4)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a == 5,"), r);
    CHECK(!status);
}
TEST(ParserFail5)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = 5,b"), r);
    CHECK(!status);
}
TEST(ParserFail6)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = {}5"), r);
    CHECK(!status);
}
TEST(ParserFail7)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = {5}"), r);
    CHECK(!status);
}
TEST(ParserFail8)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = {b = 5, c =5"), r);
    CHECK(!status);
}
TEST(ParserFail9)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = [b = 5, c =5"), r);
    CHECK(!status);
}
TEST(ParserFail10)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = {b = 5, c =5}}"), r);
    CHECK(!status);
}
TEST(ParserFail11)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = [b = 5, c =5]]"), r);
    CHECK(!status);
}
TEST(ParserFail12)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = [b = 5, c =5}"), r);
    CHECK(!status);
}
TEST(ParserFail13)
{
    dbg_mi::ResultValue r;
    bool status = dbg_mi::ParseValue(_T("a = {b = 5, c =5]"), r);
    CHECK(!status);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(ResultParse_TestType)
{
    dbg_mi::ResultParser parser;
    CHECK(parser.Parse(_T("^done,a = 5, b = 6")));

    CHECK_EQUAL(parser.GetResultType(), dbg_mi::ResultParser::Result);
}

TEST(ResultParse_TestClass)
{
    dbg_mi::ResultParser parser;
    CHECK(parser.Parse(_T("^done,a = 5, b = 6")));

    CHECK(parser.GetResultClass() == dbg_mi::ResultParser::ClassDone);
}
TEST(ResultParse_NoClassComma)
{
    dbg_mi::ResultParser parser;
    bool r = parser.Parse(_T("^done a = 5, b = 6"));

    CHECK(!r);
}

TEST(ResultParse_TestValues)
{
    dbg_mi::ResultParser parser;
    parser.Parse(_T("^done,a = 5, b = 6"));

    dbg_mi::ResultValue const &v = parser.GetResultValue();

    CHECK(v.GetType() == dbg_mi::ResultValue::Tuple && v.GetTupleSize() == 2);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TestTupleValueLookup
{
    TestTupleValueLookup()
    {
        status = dbg_mi::ParseValue(_T("a = {b=5,c=6,d=7}"), result);
    }
    dbg_mi::ResultValue result;
    bool status;
};

TEST_FIXTURE(TestTupleValueLookup, Status)
{
    CHECK(status);
}
TEST_FIXTURE(TestTupleValueLookup, TestAB)
{
    const dbg_mi::ResultValue *ab = result.GetTupleValue(wxT("a.b"));
    CHECK(ab && ab->GetType() == dbg_mi::ResultValue::Simple && ab->GetSimpleValue() == wxT("5"));
}
TEST_FIXTURE(TestTupleValueLookup, TestAC)
{
    const dbg_mi::ResultValue *ac = result.GetTupleValue(wxT("a.c"));
    CHECK(ac && ac->GetType() == dbg_mi::ResultValue::Simple && ac->GetSimpleValue() == wxT("6"));
}
TEST_FIXTURE(TestTupleValueLookup, TestAD)
{
    const dbg_mi::ResultValue *ad = result.GetTupleValue(wxT("a.d"));
    CHECK(ad && ad->GetType() == dbg_mi::ResultValue::Simple && ad->GetSimpleValue() == wxT("7"));
}
TEST_FIXTURE(TestTupleValueLookup, TestMissing)
{
    const dbg_mi::ResultValue *a = result.GetTupleValue(wxT("a.missing"));
    CHECK(!a);
}

TEST(TestTupleValueLookup2)
{
    dbg_mi::ResultValue result;
    bool status = dbg_mi::ParseValue(_T("a = {b=5,c=6,d=7}"), result);
    const dbg_mi::ResultValue *r = result.GetTupleValue(wxT("a.b.c"));
    CHECK(status && !r);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(TestResultParser_TypeResult)
{
    dbg_mi::ResultParser::Type type = dbg_mi::ResultParser::ParseType(wxT("^running"));
    CHECK_EQUAL(dbg_mi::ResultParser::Result, type);
}

TEST(TestResultParser_TypeExecAsyncOutput)
{
    dbg_mi::ResultParser::Type type = dbg_mi::ResultParser::ParseType(wxT("*stopped,reason=\"breakpoint-hit\""));
    CHECK_EQUAL(dbg_mi::ResultParser::ExecAsyncOutput, type);
}

TEST(TestResultParser_TypeStatusAsyncOutput)
{
    dbg_mi::ResultParser::Type type;
    type = dbg_mi::ResultParser::ParseType(wxT("+download,{section=\".text\",")
                                           wxT("section-size=\"6668\",total-size=\"9880\"}"));
    CHECK_EQUAL(dbg_mi::ResultParser::StatusAsyncOutput, type);
}

TEST(TestResultParser_TypeNotifyAsyncOutput)
{
    dbg_mi::ResultParser::Type type;
    type = dbg_mi::ResultParser::ParseType(wxT("=some-notification"));
    CHECK_EQUAL(dbg_mi::ResultParser::NotifyAsyncOutput, type);
}

TEST(TestResultParser_TypeUnknown)
{
    dbg_mi::ResultParser::Type type = dbg_mi::ResultParser::ParseType(wxT("stopped,reason=\"breakpoint-hit\""));
    CHECK_EQUAL(dbg_mi::ResultParser::TypeUnknown, type);
}

TEST(TestResultParser_Running)
{
    dbg_mi::ResultParser parser;

    CHECK(parser.Parse(wxT("^running")));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(ResultValueAssigmentOperator)
{
    dbg_mi::ResultValue v1, v2;

    dbg_mi::ParseValue(_T("a = 5, b = 6"), v1);
    v2 = v1;

    CHECK(v1.MakeDebugString() == v2.MakeDebugString());
}

TEST(ResultValueCopyCtor)
{
    dbg_mi::ResultValue v1;

    dbg_mi::ParseValue(_T("a = 5, b = 6"), v1);
    dbg_mi::ResultValue v2(v1);

    CHECK(v1.MakeDebugString() == v2.MakeDebugString());
}

TEST(ResultValueEquallityOperator)
{
    dbg_mi::ResultValue v1;
    dbg_mi::ParseValue(_T("a = 5, b = 6"), v1);
    dbg_mi::ResultValue v2(v1);

    CHECK(v1 == v2);
}

TEST(ResultValueInequallityOperator)
{
    dbg_mi::ResultValue v1, v2;
    dbg_mi::ParseValue(_T("a = 5, b = 6"), v1);
    dbg_mi::ParseValue(_T("a = 5, c = 6"), v1);

    CHECK(v1 != v2);
}

TEST(ResultParserAssigmentOperator)
{
    dbg_mi::ResultParser p1, p2;
    bool r1 = p1.Parse(_T("^done,a = 5, b = 6"));
    p2 = p1;

    CHECK(r1);
    CHECK(p1.MakeDebugString() == p2.MakeDebugString());
}
TEST(ResultParserCopyCtor)
{
    dbg_mi::ResultParser p1;
    bool r1 = p1.Parse(_T("^done,a = 5, b = 6"));
    dbg_mi::ResultParser p2(p1);

    CHECK(r1);
    CHECK(p1.MakeDebugString() == p2.MakeDebugString());
}

TEST(ResultParserEquallityOperator)
{
    dbg_mi::ResultParser p1, p2;
    bool r1 = p1.Parse(_T("^done,a = 5, b = 6"));
    bool r2 = p2.Parse(_T("^done,a = 5, b = 6"));

    CHECK(r1 && r2);
    CHECK(p1 == p2);
}

TEST(ResultParserInequallityOperator)
{
    dbg_mi::ResultParser p1, p2;
    bool r1 = p1.Parse(_T("^done,a = 5, b = 6"));
    bool r2 = p2.Parse(_T("^done,a = 5, c = 6"));

    CHECK(r1 && r2);
    CHECK(p1 != p2);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(LookupInt)
{
    dbg_mi::ResultValue result_value;
    dbg_mi::ParseValue(wxT("a = 5, b = 6"), result_value, 0);
    int value = 0;
    CHECK(dbg_mi::Lookup(result_value, wxT("a"), value) && value == 5);
}

TEST(LookupString)
{
    dbg_mi::ResultValue result_value;
    dbg_mi::ParseValue(wxT("a = 5, b = 6"), result_value, 0);
    wxString value;
    CHECK(dbg_mi::Lookup(result_value, wxT("a"), value) && value == wxT("5"));
}

TEST(LookupStringFail)
{
    dbg_mi::ResultValue result_value;
    dbg_mi::ParseValue(wxT("a = 5, b = 6"), result_value, 0);
    wxString value;
    CHECK(!dbg_mi::Lookup(result_value, wxT("c"), value));
}

TEST(ToInt)
{
    dbg_mi::ResultValue result_value;
    dbg_mi::ParseValue(wxT("a = 5, b = 6"), result_value, 0);
    int value = 0;
    dbg_mi::ResultValue const *b = result_value.GetTupleValue(wxT("b"));
    CHECK(b && dbg_mi::ToInt(*b, value) && value == 6);
}
