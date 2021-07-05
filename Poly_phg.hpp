#pragma once
// ======================================================================
// PMHG
// ======================================================================
#undef var
#undef INVALIDVAR

#define var			PMHG::EDGE
#define INVALIDVAR	PMHG::EDGE(0)

#undef PHGPRINT
#define PHGPRINT	PMHG::_PHGPRINT

#define PHG_VAR(name, defaultval) (PMHG::gcode.varmapstack.stack.empty() || PMHG::gcode.varmapstack.stack.front().find(#name) == PMHG::gcode.varmapstack.stack.front().end() ? defaultval : PMHG::gcode.varmapstack.stack.front()[#name])
#define PHG_PARAM(index)	cd.valstack.get(args - index)

extern void initphg();

namespace PMHG
{
	// 运算
	struct EDGE
	{
		enum {INT, REAL, QUAT, VLIST};
		byte type = REAL;
		union {
			int ival;
			real fval;
			quaternion q;
		};
		VECLIST vlist;

		EDGE() {
			type = VLIST;
		}
		EDGE(int _val)
		{
			type = INT;
			ival = _val;
		}
		EDGE(real _val)
		{
			type = REAL;
			fval = _val;
		}
		EDGE(const quaternion& _q)
		{
			type = QUAT;
			q = _q;
		}
		EDGE(const EDGE& v) {
			type = v.type;
			if (type == REAL)
				fval = v.fval;
			else if (type == QUAT)
				q = v.q;
			else if (type == VLIST)
				vlist = v.vlist;
		}
		operator const real& ()
		{
			return fval;
		}
		bool operator==(const EDGE& v) const
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
			return ival == v;
		}
		bool operator==(real v) const
		{
			return fval == v;
		}
		EDGE operator + (const EDGE& v) const
		{
			EDGE e;
			if (type == REAL && v.type == type)
			{
				e.type = REAL;
				e.fval = fval + v.fval;
			}
			else if (type == INT && v.type == type)
			{
				e.type = INT;
				e.ival = ival + v.ival;
			}
			else if (type == VLIST && v.type == VLIST)
			{
				e.type = VLIST;
				if (vlist.empty())
					return v;
				
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
			}
			else if (type == VLIST && v.type == QUAT)
			{
				e.type = VLIST;
				e.vlist = vlist;
				for (int i = 0; i < e.vlist.size(); i++)
				{
					e.vlist[i] = e.vlist[i] + v.q.xyz();
				}
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
			if (type == VLIST && v.type == VLIST)
			{
				PRINT("* VLIST");
				vec3 n = vec3::UY;
				vec3 lsp;
				e.vlist = vlist;
				for (int j = 0; j < v.vlist.size(); j++)
				{
					vec3 p = v.vlist[j].p - lsp;
					real r = p.len();

					real rr = p.lenxz();
					if (rr > 0)
					{
						real ang = atan2(p.z / rr, p.x / rr);
						PRINTV(ang);
						for (int i = 0; i < e.vlist.size(); i++)
						{
							e.vlist[i].p.rot(ang, n);
							//e.vlist[i].p *= r;
						}
					}
					lsp = v.vlist[j].p;
				}
			}
			else if (type == VLIST && v.type == REAL)
			{
				int len = int(v.fval);
				PRINT("* REAL " << v.fval);
				for (int i = 0; i < len; i++)
					e = e + (*this);
				
			}
			else if (type == VLIST && v.type == QUAT)
			{
				PRINT("* QUAT");
				e.vlist = vlist;
				for (int i = 0; i < e.vlist.size(); i++)
				{
					quaternion q;
					q.fromangleaxis(v.q.w * PI / 180.0f, v.q.xyz().normcopy());
					
					e.vlist[i].p =  q * (e.vlist[i].p * v.q.xyz().len());
				}
			}
			else if (type == REAL && v.type == REAL)
			{
				e.type = REAL;
				PRINT("* REAL REAL")
				e.fval = e.fval * v.fval;
			}
			return e;
		}
		EDGE operator / (const EDGE& v) const
		{
			EDGE e;
			if (type == VLIST && v.type == REAL)
			{
				PRINT("/ REAL");

				int len = int(e.vlist.size() / v.fval);
				for (int i = 0; i < len; i++)
					e.vlist.push_back(vlist[i]);
			}
			return e;
		}
	};

	inline void _PHGPRINT(const EDGE& e)
	{
		if(e.type == EDGE::REAL)
			PRINT(e.fval);
		for (auto& i : e.vlist)
			PRINTVEC3(i.p)
	}

	#include "phg.hpp"

	// act
	var _act(code& cd, int args)
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
		case '/': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a / b;
		}
		case '>': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			
			return var(a.fval > b.fval ? 1.0f : 0.0f);
		}
		case '<': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();

			return var(a.fval < b.fval ? 1.0f : 0.0f);
		}
		default: {return 0; }
		}
	}

	inline real chars2real(code& cd)
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

		return atof(buff);
	}

	// table
	static void _table(code& cd)
	{
		PRINT("TABLE: " << gtable.size() + 1);

		cd.next();

		var edge;
		quaternion q;
		q.w = 0;
		int vcnt = 0;

		while (!cd.eoc()) {
			char c = cd.cur();
			if (c == ']') {
				PRINTVEC4(q);
				//PRINTV(vcnt);
				if (vcnt == 3)
				{
					edge.type = EDGE::VLIST;
					edge.vlist.push_back(q.xyz());
				}
				else if (vcnt == 4)
				{
					edge.type = EDGE::QUAT;
					edge.q = q;
				}

				gtable.push_back(edge);

				edge.vlist.clear();

				cd.next();
				cd.next();
				break;
			}
			else if (c == ';') {
				PRINTVEC4(q);
				//PRINTV(vcnt);
				if (vcnt == 3)
				{
					edge.vlist.push_back(q.xyz());
				}
				else if(vcnt == 4)
				{
					edge.q = q;
				}

				cd.next();
				continue;
			}
			else {
				vcnt = 0;
				short type = get(cd);
				if (type == OPR || type == NUMBER)
				{
					real neg;
					{
						neg = 1.0f;
						if ('-' == cd.cur())
						{
							neg = -1.0f;
							cd.next();
						}
					}
					q.x = neg * chars2real(cd);
					vcnt++;
				}
				if (cd.cur() != ';' && cd.cur() != ']')
				{
					cd.next();
					real neg;
					{
						neg = 1.0f;
						if ('-' == cd.cur())
						{
							neg = -1.0f;
							cd.next();
						}
					}
					q.y = neg * chars2real(cd);
					vcnt++;
				}
				if (cd.cur() != ';' && cd.cur() != ']')
				{
					cd.next();
					real neg;
					{
						neg = 1.0f;
						if ('-' == cd.cur())
						{
							neg = -1.0f;
							cd.next();
						}
					}
					q.z = neg * chars2real(cd);
					vcnt++;
				}
				if (cd.cur() != ';' && cd.cur() != ']')
				{
					cd.next();
					real neg;
					{
						neg = 1.0f;
						if ('-' == cd.cur())
						{
							neg = -1.0f;
							cd.next();
						}
					}
					q.w = neg * chars2real(cd);
					vcnt++;
				}
			}
		}
	}
	
	void setup()
	{
		initphg();

		table = _table;
		act = _act;
		
		register_api("render",
			[](code& cd, int args)->var {
				estack.clear();
				for (auto& it : gtable)
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

// ---------------------------------------------------------------
static var pushe(PMHG::code& cd, int args)
{
	//PRINTV(PHG_PARAM(1).vlist.size());
	if (args == 1)
	{
		estack.push_back(PHG_PARAM(1).vlist);
	}
	else
	{
		if (estack.empty())
			estack.push_back(VECLIST());
		else
		{
			estack.push_back(estack.back());
		}
	}

	return 0;
}
static var pope(PMHG::code& cd, int args)
{
	estack.pop_back();

	return 0;
}

static var extrudeedge(PMHG::code& cd, int args)
{
	float d = 1;

	VECLIST& e1 = estack.back();
	vec norm = getedgenorm2(e1);
	vec dv = norm * d;
	{
		for (int i = 0; i < e1.size(); i++)
		{
			e1[i].p += dv;
			e1[i].ind = -1;
		}
	}
	return INVALIDVAR;
}

static var facepoly(PMHG::code& cd, int args)
{
	if (estack.empty())
		return INVALIDVAR;

	std::vector<vec3> tris;
	POLY::link_tri(estack.back(), vec3::UX, tris);
	for (int i = 0; i < tris.size(); i += 3)
	{
		triang0(tris[i], tris[i + 1], tris[i + 2]);
	}
	return INVALIDVAR;
}

static var face(PMHG::code& cd, int args)
{
	face(estack[estack.size() - 2], estack.back());
	return 0;
}

//------------------------------------------
// init
//------------------------------------------
static void initphg()
{
	PRINT("setup PMHG");
	
	reset();

	PMHG::register_api("push", pushe);
	PMHG::register_api("pop", pope);

	PMHG::register_api("ext", extrudeedge);

	PMHG::register_api("face", face);

	PMHG::register_api("facepoly", facepoly);
}
// ======================================================================
// test
// ======================================================================

void test()
{
	PMHG::dofile("main.e");
}
