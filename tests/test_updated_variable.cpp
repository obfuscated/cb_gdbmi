#include <UnitTest++.h>

#include "cmd_result_parser.h"
#include "updated_variable.h"

namespace {
//
//struct FixtureUpdateVar
//{
//    FixtureUpdateVar()
//    {
//        status = false;
//    }
//
//    dbg_mi::UpdatedVariable var;
//    bool status;
//};
//
//TEST_FIXTURE(FixtureUpdateVar, Status)
//{
////    CHECK(status);
//};


wxString const c_simple(wxT("s = {name=\"var1.private.c\",value=\"5\",")
                        wxT("in_scope=\"true\",type_changed=\"false\",has_more=\"0\"}"));

wxString const c_new_type_int(wxT("s={name=\"var1\",value=\"0\",in_scope=\"true\",")
                              wxT("type_changed=\"true\",new_type=\"int\",new_num_children=\"0\",has_more=\"0\"}"));
struct Fixture
{
    Fixture(wxString const &s)
    {
        status = false;
        dbg_mi::ResultValue value;
        if(!dbg_mi::ParseValue(s, value, 0))
            return;
        dbg_mi::ResultValue const *sub_value = value.GetTupleValue(wxT("s"));
        if(sub_value)
            status = var.Parse(*sub_value);
    }
    bool status;
    dbg_mi::UpdatedVariable var;
};

TEST(UpdatedVar_Simple_Status)
{
    Fixture f(c_simple);
    CHECK(f.status);
}

TEST(UpdatedVar_Simple_InScope)
{
    Fixture f(c_simple);
    CHECK_EQUAL(dbg_mi::UpdatedVariable::InScope_Yes, f.var.GetInScope());
}

TEST(UpdatedVar_Simple_Name)
{
    Fixture f(c_simple);
    CHECK_EQUAL(wxT("var1.private.c"), f.var.GetName());
}

TEST(UpdatedVar_Simple_TypeChanged)
{
    Fixture f(c_simple);
    CHECK(!f.var.TypeChanged());
}

TEST(UpdatedVar_Simple_Value)
{
    Fixture f(c_simple);
    CHECK(f.var.HasValue());
    CHECK_EQUAL(wxT("5"), f.var.GetValue());
}

///////////////////////////////////////////////////////////////////////////////////////
TEST(UpdatedVar_NewTypeInt_Status)
{
    Fixture f(c_new_type_int);
    CHECK(f.status);
}

TEST(UpdatedVar_NewTypeInt_TypeChanged)
{
    Fixture f(c_new_type_int);
    CHECK(f.var.TypeChanged());
}

TEST(UpdatedVar_NewTypeInt_NewType)
{
    Fixture f(c_new_type_int);
    CHECK_EQUAL(wxT("int"), f.var.GetNewType());
}

TEST(UpdatedVar_NewTypeInt_NewNumChildren)
{
    Fixture f(c_new_type_int);
    CHECK_EQUAL(0, f.var.GetNewNumberOfChildren());
    CHECK(f.var.HasNewNumberOfChildren());
}

} // nameless namespace
