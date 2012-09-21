#include <UnitTest++.h>

#include "definitions.h"

cb::shared_ptr<dbg_mi::Watch> MakeWatch(wxString const &symbol, wxString const &exp)
{
    cb::shared_ptr<dbg_mi::Watch> w(new dbg_mi::Watch(symbol, false));
    w->SetID(exp);
    return w;
}

struct FindWatchFixture
{
    FindWatchFixture()
    {
        {
            cb::shared_ptr<dbg_mi::Watch> w(MakeWatch(wxT("var1"), wxT("var1")));
            cb::shared_ptr<dbg_mi::Watch> w_c = MakeWatch(wxT("var1.c"), wxT("var1.public.c"));
            cbWatch::AddChild(w, w_c);

            cb::shared_ptr<dbg_mi::Watch> w_c_a = MakeWatch(wxT("var1.c.a"), wxT("var1.public.c.private.a"));
            cbWatch::AddChild(w_c, w_c_a);

            watches.push_back(w);
        }
        {
            cb::shared_ptr<dbg_mi::Watch> w(MakeWatch(wxT("var2"), wxT("var2")));
            cb::shared_ptr<dbg_mi::Watch> w_b = MakeWatch(wxT("var2.b"), wxT("var2.public.b"));
            cbWatch::AddChild(w, w_b);

            cb::shared_ptr<dbg_mi::Watch> w_b_a = MakeWatch(wxT("var2.c.a"), wxT("var2.public.b.private.a"));
            cbWatch::AddChild(w_b, w_b_a);

            cb::shared_ptr<dbg_mi::Watch> w_b_b = MakeWatch(wxT("var2.c.b"), wxT("var2.public.b.private.b"));
            cbWatch::AddChild(w_b, w_b_b);
            watches.push_back(w);
        }
        watches.push_back(cb::shared_ptr<dbg_mi::Watch>(MakeWatch(wxT("var3"), wxT("var3"))));
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
    cb::shared_ptr<dbg_mi::Watch> w = dbg_mi::FindWatch(wxT("var1"), watches);
    CHECK(w && w->GetID() == wxT("var1"));
}

TEST_FIXTURE(FindWatchFixture, Var2)
{
    cb::shared_ptr<dbg_mi::Watch> w = dbg_mi::FindWatch(wxT("var2"), watches);
    CHECK(w && w->GetID() == wxT("var2"));
}

TEST_FIXTURE(FindWatchFixture, Var3)
{
    cb::shared_ptr<dbg_mi::Watch> w = dbg_mi::FindWatch(wxT("var3"), watches);
    CHECK(w && w->GetID() == wxT("var3"));
}

TEST_FIXTURE(FindWatchFixture, Var1_public_c)
{
    cb::shared_ptr<dbg_mi::Watch> w = dbg_mi::FindWatch(wxT("var1.public.c"), watches);
    CHECK(w && w->GetID() == wxT("var1.public.c"));
}

TEST_FIXTURE(FindWatchFixture, Var1_public_c_private_a)
{
    cb::shared_ptr<dbg_mi::Watch> w = dbg_mi::FindWatch(wxT("var1.public.c.private.a"), watches);
    CHECK(w && w->GetID() == wxT("var1.public.c.private.a"));
}

TEST_FIXTURE(FindWatchFixture, Var1_public_b)
{
    cb::shared_ptr<dbg_mi::Watch> w = dbg_mi::FindWatch(wxT("var2.public.b"), watches);
    CHECK(w && w->GetID() == wxT("var2.public.b"));
}

TEST_FIXTURE(FindWatchFixture, Var1_public_b_private_a)
{
    cb::shared_ptr<dbg_mi::Watch> w = dbg_mi::FindWatch(wxT("var2.public.b.private.a"), watches);
    CHECK(w && w->GetID() == wxT("var2.public.b.private.a"));
}

TEST_FIXTURE(FindWatchFixture, Var1_public_b_private_b)
{
    cb::shared_ptr<dbg_mi::Watch> w = dbg_mi::FindWatch(wxT("var2.public.b.private.b"), watches);
    CHECK(w && w->GetID() == wxT("var2.public.b.private.b"));
}
