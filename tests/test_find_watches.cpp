#include <UnitTest++.h>

#include "definitions.h"

dbg_mi::Watch::Pointer MakeWatch(wxString const &symbol, wxString const &exp)
{
    dbg_mi::Watch::Pointer w(new dbg_mi::Watch(symbol, false));
    w->SetID(exp);
    return w;
}

struct FindWatchFixture
{
    FindWatchFixture()
    {
        {
            dbg_mi::Watch::Pointer w(MakeWatch(wxT("var1"), wxT("var1")));
            dbg_mi::Watch::Pointer w_c = MakeWatch(wxT("var1.c"), wxT("var1.public.c"));
            cbWatch::AddChild(w, w_c);

            dbg_mi::Watch::Pointer w_c_a = MakeWatch(wxT("var1.c.a"), wxT("var1.public.c.private.a"));
            cbWatch::AddChild(w_c, w_c_a);

            watches.push_back(w);
        }
        {
            dbg_mi::Watch::Pointer w(MakeWatch(wxT("var2"), wxT("var2")));
            dbg_mi::Watch::Pointer w_b = MakeWatch(wxT("var2.b"), wxT("var2.public.b"));
            cbWatch::AddChild(w, w_b);

            dbg_mi::Watch::Pointer w_b_a = MakeWatch(wxT("var2.c.a"), wxT("var2.public.b.private.a"));
            cbWatch::AddChild(w_b, w_b_a);

            dbg_mi::Watch::Pointer w_b_b = MakeWatch(wxT("var2.c.b"), wxT("var2.public.b.private.b"));
            cbWatch::AddChild(w_b, w_b_b);
            watches.push_back(w);
        }
        watches.push_back(dbg_mi::Watch::Pointer(MakeWatch(wxT("var3"), wxT("var3"))));
    }

    void AddWatch(wxString const &exp)
    {
        dbg_mi::Watch *w = new dbg_mi::Watch(exp, false);
        w->SetID(exp);
        watches.push_back(std::tr1::shared_ptr<dbg_mi::Watch>(w));
    }

    dbg_mi::WatchesContainer watches;
};

TEST_FIXTURE(FindWatchFixture, Var1)
{
    dbg_mi::Watch::Pointer w = dbg_mi::FindWatch(wxT("var1"), watches);
    CHECK(w && w->GetID() == wxT("var1"));
}

TEST_FIXTURE(FindWatchFixture, Var2)
{
    dbg_mi::Watch::Pointer w = dbg_mi::FindWatch(wxT("var2"), watches);
    CHECK(w && w->GetID() == wxT("var2"));
}

TEST_FIXTURE(FindWatchFixture, Var3)
{
    dbg_mi::Watch::Pointer w = dbg_mi::FindWatch(wxT("var3"), watches);
    CHECK(w && w->GetID() == wxT("var3"));
}

TEST_FIXTURE(FindWatchFixture, Var1_public_c)
{
    dbg_mi::Watch::Pointer w = dbg_mi::FindWatch(wxT("var1.public.c"), watches);
    CHECK(w && w->GetID() == wxT("var1.public.c"));
}

TEST_FIXTURE(FindWatchFixture, Var1_public_c_private_a)
{
    dbg_mi::Watch::Pointer w = dbg_mi::FindWatch(wxT("var1.public.c.private.a"), watches);
    CHECK(w && w->GetID() == wxT("var1.public.c.private.a"));
}

TEST_FIXTURE(FindWatchFixture, Var1_public_b)
{
    dbg_mi::Watch::Pointer w = dbg_mi::FindWatch(wxT("var2.public.b"), watches);
    CHECK(w && w->GetID() == wxT("var2.public.b"));
}

TEST_FIXTURE(FindWatchFixture, Var1_public_b_private_a)
{
    dbg_mi::Watch::Pointer w = dbg_mi::FindWatch(wxT("var2.public.b.private.a"), watches);
    CHECK(w && w->GetID() == wxT("var2.public.b.private.a"));
}

TEST_FIXTURE(FindWatchFixture, Var1_public_b_private_b)
{
    dbg_mi::Watch::Pointer w = dbg_mi::FindWatch(wxT("var2.public.b.private.b"), watches);
    CHECK(w && w->GetID() == wxT("var2.public.b.private.b"));
}
