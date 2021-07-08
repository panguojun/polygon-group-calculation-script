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
		enum {REAL, QUAT, VLIST};
		byte type = REAL;
		union {
			real fval;
			quaternion q;
		};
		VECLIST vlist;

		EDGE() {
			type = VLIST;
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
			if (type == VLIST && v.type == type)
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
			if (type == REAL && v.type == type)
			{
				return fval == v.fval;
			}
			if (type == QUAT && v.type == type)
			{
				return q == v.q;
			}
			return false;
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
				PRINT("+ REAL REAL");
				e.type = REAL;
				e.fval = fval + v.fval;
			}
			else if (type == VLIST && v.type == VLIST)
			{
				PRINT("+ VLIST VLIST");

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
				PRINT("+ VLIST QUAT");

				e.type = VLIST;
				e.vlist = vlist;
				for (int i = 0; i < e.vlist.size(); i++)
				{
					e.vlist[i] = e.vlist[i] + v.q.xyz();
				}
			}
			else if (type == QUAT && v.type == QUAT)
			{
				ERRORMSG("+ QUAT QUAT");
			}
			else
			{
				ERRORMSG("+ ERROR");
			}
			return e;
		}
		EDGE operator - (const EDGE& v) const
		{
			if (type == VLIST && v.type == VLIST)
			{
				PRINT("- VLIST VLIST");
				return (*this) + (-v);
			}
			if (type == VLIST && v.type == REAL)
			{
				PRINT("- VLIST REAL");
				return (*this) + (-v);
			}
			else if (type == VLIST && v.type == QUAT)
			{
				PRINT("- VLIST QUAT");
				return (*this) + (-v);
			}
			else
			{
				ERRORMSG("- ERROR");
			}
			return INVALIDVAR;
		}
		EDGE operator - () const
		{
			EDGE e;
			e.type = type;
			if (type == VLIST)
			{
				for (int i = 0; i < vlist.size(); i++)
				{
					e.vlist.push_back(-vlist[i].p);
				}
			}
			else if (type == REAL)
			{
				e.fval = -fval;
			}
			else if (type == QUAT)
			{
				e.q = -q;
			}
			return e;
		}
		EDGE operator * (const EDGE& v) const
		{
			EDGE e;
			if (type == VLIST && v.type == VLIST)
			{
				ERRORMSG("* VLIST VLIST");
			}
			else if (type == VLIST && v.type == REAL)
			{
				e.type = VLIST;
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
				PRINT("* REAL REAL");
				e.fval = e.fval * v.fval;
			}
			else if (type == REAL && v.type == VLIST)
			{
				ERRORMSG("* REAL VLIST");
			}
			else if (type == QUAT && v.type == VLIST)
			{
				ERRORMSG("* QUAT VLIST");
			}
			else if (type == QUAT && v.type == QUAT)
			{
				e.type = QUAT;
				PRINT("* QUAT QUAT");
				e.q = q * v.q;
			}
			else
			{
				ERRORMSG("* ERROR");
			}
			return e;
		}
		EDGE operator / (const EDGE& v) const
		{
			EDGE e;
			if (type == VLIST && v.type == REAL)
			{
				PRINT("/ VLIST REAL");

				int len = int(e.vlist.size() / v.fval);
				for (int i = 0; i < len; i++)
				
					e.vlist.push_back(vlist[i]);
			}
			else
			{
				ERRORMSG("/ ERROR");
			}
			return e;
		}
		inline void merge(VECLIST& out, const VECLIST& e1, int pos1, const VECLIST& e2, int pos2) const
		{
			for (; pos1 < e1.size(); pos1++)
			{
				for (int j = pos2; j < e2.size(); j++)
				{
					if (e1[pos1].p == e2[j].p)
					{
						PRINT("merg2e " << pos1 << "," << pos1);
						out.push_back(e1[pos1]);
						return merge(out, e2, j + 1, e1, pos1 + 1);
					}
				}
				out.push_back(e1[pos1]);
			}
		}
		EDGE operator | (const EDGE& v) const
		{
			EDGE e;
			if (type == VLIST && v.type == VLIST)
			{
				PRINT("| VLIST VLIST");

				e.type = VLIST;

				if (vlist.empty())
					return v;
				
				merge(e.vlist, vlist, 0, v.vlist, 0);
				PRINTV(e.vlist.size());
			}

			else if (type == REAL && v.type == REAL)
			{
				PRINT("| REAL REAL");
				return fval || v.fval;
			}
			else
			{
				ERRORMSG("| ERROR");
			}
			return e;
		}
		EDGE operator & (const EDGE& v) const
		{
			EDGE e;
			if (type == VLIST && v.type == VLIST)
			{
				PRINT("& VLIST VLIST");

				e.type = VLIST;

				if (vlist.empty())
					return v;

				for (int i = 0; i < v.vlist.size(); i++)
				{
					for (int j = 0; j < vlist.size(); j++)
					{
						if (v.vlist[i].p == vlist[j].p)
							e.vlist.push_back(v.vlist[i]);
					}
				}
			}
			else if (type == REAL && v.type == REAL)
			{
				PRINT("& REAL REAL");
				return fval && v.fval;
			}
			else
			{
				ERRORMSG("| error");
			}
			return e;
		}
	};

	inline void _PHGPRINT(const std::string& pre, const EDGE& e)
	{
		if (e.type == EDGE::REAL)
		{
			PRINT(pre << e.fval);
		}
		else if (e.type == EDGE::QUAT)
		{
			quaternion q;
			q.fromangleaxis(e.q.w * PI / 180.0f, e.q.xyz().normcopy());
			PRINT(pre << q.x << "," << q.y << "," << q.z << "," << q.w)
		}
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
		case '|': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a | b;
		}
		case '&': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a & b;
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
		case '!': {
			if (args > 1) {
				var b = cd.valstack.pop();
				var a = cd.valstack.pop();
				return !(b == a);
			}
			else
			{
				var a = cd.valstack.pop();
				return !a;
			}
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
		PRINT("TABLE: " << gtable.size());

		cd.next();

		var edge;
		quaternion q;
		q.w = 0;
		int vcnt = 0;

		while (!cd.eoc()) {
			char c = cd.cur();
			if (c == ']') {
				//PRINTV(vcnt);
				if (vcnt == 3)
				{
					PRINTVEC3(q.xyz());
					edge.type = EDGE::VLIST;
					edge.vlist.push_back(q.xyz());
				}
				else if (vcnt == 4)
				{
					PRINTVEC3(q);
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
				//PRINTV(vcnt);
				if (vcnt == 3)
				{
					PRINTVEC3(q.xyz());
					edge.vlist.push_back(q.xyz());
				}
				else if(vcnt == 4)
				{
					PRINTVEC4(q);
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
		{
			table = _table;
			// zero element
			gtable.clear();
			EDGE e; e.vlist.push_back(vec3::ZERO);
			gtable.push_back(e);
		}
		act = _act;
	}
}

// ---------------------------------------------------------------
static var pushe(PMHG::code& cd, int args)
{
	//PRINTV(PHG_PARAM(1).vlist.size());
	if (args == 1)
	{
		estack.push_back(PHG_PARAM(1).vlist);
		//closeedge(estack.back());
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
	ASSERT(args == 1);

	float d = PHG_PARAM(1).fval;

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
static var face0(PMHG::code& cd, int args)
{
	CUTSM::drawclosededge(estack.back());

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
	PMHG::register_api("face0", face0);

	PMHG::register_api("facepoly", facepoly);
}
// ======================================================================
// test
// ======================================================================

void test()
{
	{// zero element
		PMHG::gtable.clear();
		PMHG::EDGE e; e.vlist.push_back(vec3::ZERO);
		PMHG::gtable.push_back(e);
	}

	PMHG::dofile("main.e");
}
