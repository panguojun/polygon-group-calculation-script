#pragma once
// ======================================================================
// EDGE PHG
// ======================================================================

#define var			EDGE
#define INVALIDVAR	EDGE(0)

struct EDGE
{
	int val = 0;
	VECLIST vlist;
	EDGE() {}
	EDGE(int _val)
	{
		val = _val;
	}
	operator const int& ()
	{
		return val;
	}
	bool operator==(EDGE v) const
	{
		if (vlist.size() != v.vlist.size())
			return false;
		for (int i = 0; i < vlist.size(); i++)
		{
			if (vlist[i].p != v.vlist[i].p)
				return false;
		}
		return true;
	}
	bool operator==(int v) const
	{
		return val == v;
	}
	EDGE operator + (const EDGE& v) const
	{
		if (vlist.empty())
			return v;

		EDGE e;
		e.vlist = vlist;
		vec3 lastp = vec3::ZERO;
		for (auto it : v.vlist)
		{
			vec3 np = e.vlist.back().p + (it.p - lastp);
			lastp = it.p;

			bool bloop = false;
			for (int i = 0; i < e.vlist.size(); i++)
			{
				if (np == e.vlist[i].p)
				{
					e.vlist.assign(e.vlist.begin(), e.vlist.begin() + i + 1);
					bloop = true;
					break;
				}
			}
			if (!bloop)
				e.vlist.push_back(np);
		}
		return e;
	}
	EDGE operator - (const EDGE& v) const
	{
		return (*this) + (-v);
	}
	EDGE operator - () const
	{
		EDGE e;
		for (int i = 0; i < vlist.size(); i++)
		{
			e.vlist.push_back(-vlist[i].p);
		}
		return e;
	}
	EDGE operator * (const EDGE& v) const
	{
		EDGE e;

		e.vlist = vlist;
		int len = _MIN(v.vlist.size(), vlist.size());
		for(int i = 0; i < len; i ++)
		{
			vec3 n = v.vlist[i].p.normcopy();
			real ang = v.vlist[i].p.len();

			e.vlist[i].p.rot(ang, n);
		}
		return e;
	}
};

#include "phg.hpp"

// ======================================================================
// PMHG
// ======================================================================
namespace PMHG
{
	inline void PHGPRINT(const EDGE& e)
	{
		for (auto& i : e.vlist)
			PRINTVEC3(i.p)
	}

	// act
	var act(PHG::code& cd, int args)
	{
		opr o = cd.oprstack.pop();

		PRINT("act: " << o << "(" << args << ")");

		switch (o) {
		case '+': {
			if (args > 1) {
				var b = cd.valstack.pop();
				var a = cd.valstack.pop();
				return a + b;
			}
			else {
				return cd.valstack.pop();
			}
		}
		case '-': {
			if (args > 1) {
				var b = cd.valstack.pop();
				var a = cd.valstack.pop();
				return a - b;
			}
			else {
				return -cd.valstack.pop();
			}
		}
		case '*': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a * b;
		}
		default: {return 0; }
		}
	}

	inline real chars2real(PHG::code& cd)
	{
		static char buff[128];
		int i = 0;
		for (; i < 128; i++) {
			char c = cd.cur();
			if (!isdigit(c) && c != '.')
				break;

			buff[i] = c;
			if(c != ']')
				cd.next();
		}
		buff[i] = '\0';

		return atoi(buff);
	}

	// table
	static void table(PHG::code& cd)
	{
		PRINT("table: ");

		cd.next();

		var edge;
		vec3 p;

		while (!cd.eoc()) {
			char c = cd.cur();
			if (c == ']') {
				PRINTVEC3(p);
				edge.vlist.push_back(p);

				PHG::gtable.push_back(edge);
				PRINTV(PHG::gtable.size());
				edge.vlist.clear();

				cd.next();
				cd.next();
				break;
			}
			else if (c == ';') {
				PRINTVEC3(p);
				edge.vlist.push_back(p);
				cd.next();
				continue;
			}
			else {
				short type = get(cd);
				if (type == NUMBER)
				{
					p.x = chars2real(cd);
					cd.next();
					p.y = chars2real(cd);
					cd.next();
					p.z = chars2real(cd);
				}
			}
		}
	}

	void setup()
	{
		PHG::table = table;
		PHG::act = act;
		
		PHG::register_api("render",
			[](PHG::code& cd, int args)->var {
				estack.clear();
				for (auto& it : PHG::gtable)
				{
					vec3 lastp;
					for (int i = 0; i < it.vlist.size(); i++)
					{
						auto& vit = it.vlist[i];
						pointi(point_t(vit.p.x * 50, vit.p.y * 50), 8, 0xFF0000FF);
						if (i != 0)
							drawlinei(lastp.xy() * 50, vit.p.xy() * 50, 0xFF0000FF);
						lastp = vit.p;
					}
					estack.push_back(it.vlist);
				}
				return INVALIDVAR;
			});
	}
}

// ======================================================================
// test
// ======================================================================
void test()
{
	PMHG::setup();

	PHG::dofile("main.phg");
}
