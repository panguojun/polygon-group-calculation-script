#pragma once
// **************************************************************************
// PMHG GROUP
// **************************************************************************
#define GROUP		PMHG
#define ELEMENT		PMHG::EDGE
//#define WANGGE_DUIQI

extern void initphg();

namespace PMHG
{
	#include "phg_head.hpp"

	// Element
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
		EDGE(VECLIST& e)
		{
			type = VLIST;
			vlist = e;
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
		bool operator == (real v) const
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
							if (i != 0)
							{
								e.vlist.assign(e.vlist.begin(), e.vlist.begin() + i + 1);
								bloop = true;
								break;
							}
						}
					}
					if (!bloop)
						e.vlist.push_back(np);
				}
			}
			else if (type == VLIST && v.type == QUAT)
			{
				ERRORMSG("+ VLIST QUAT");
			}
			else if (type == QUAT && v.type == QUAT)
			{
				ERRORMSG("+ QUAT QUAT");
			}
			else
			{
				PRINT(int(type) << "+" << int(v.type));
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
			else if (type == REAL && v.type == REAL)
			{
				PRINT("- REAL REAL");
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
		inline EDGE invert() const
		{
			PRINT("invert");
			EDGE e;
			e.type = type;
			if (type == VLIST)
			{
				for (int i = vlist.size()-1; i >= 0; i--)
				{
					e.vlist.push_back(vlist[i].p);
				}
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
				vec3 lstp;
				bool bgridalign = getedgenorm(vlist) == vec3::UY;
				for (int i = 0; i < e.vlist.size(); i++)
				{
					quaternion q;
					q.fromangleaxis(v.q.w * PI / 180.0f, v.q.xyz().normcopy());
					
					e.vlist[i].p =  q * (e.vlist[i].p * v.q.xyz().len());

					vec3 v = e.vlist[i].p - lstp;

					// 网格对齐！
					/*if(bgridalign)
					{
						fabs(v.x) < 1e-3 ? v.x = 0 : (v.x > 0 ? v.x = 1 : v.x = -1);
						fabs(v.y) < 1e-3 ? v.y = 0 : (v.y > 0 ? v.y = 1 : v.y = -1);
						fabs(v.z) < 1e-3 ? v.z = 0 : (v.z > 0 ? v.z = 1 : v.z = -1);
					}*/

					e.vlist[i].p = lstp + v;

					lstp = e.vlist[i].p;
				}
			}
			else if (type == REAL && v.type == REAL)
			{
				e.type = REAL;
				PRINT("* REAL REAL");
				e.fval = fval * v.fval;
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
				//ERRORMSG("* QUAT QUAT");

				e.type = QUAT;
				PRINT("* QUAT QUAT");
				e.q = q;
				e.q.w = v.q.w + q.w;
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
			else if (type == REAL && v.type == REAL)
			{
				e.type = REAL;
				PRINT("/ REAL REAL");
				e.fval = fval / v.fval;
			}
			else
			{
				ERRORMSG("/ ERROR");
			}
			return e;
		}
		inline bool issame(const vector3& v1, const vector3& v2) const
		{
			const real c_EPSINON = 1e-2;
			return (fabs(v1.x - v2.x) <= c_EPSINON && fabs(v1.y - v2.y) <= c_EPSINON && fabs(v1.z - v2.z) <= c_EPSINON);
		}
		inline void _merge(VECLIST& out, const VECLIST& e1, int pos1, const VECLIST& e2, int pos2) const
		{
			for (; pos1 < e1.size(); pos1++)
			{
				for (int j = pos2; j < e2.size(); j++)
				{
					PRINT(e1.size() << " pos1=" << pos1 <<";" << e2.size() << ",j=" << j);
					
					if (issame(e1[pos1].p, e2[j].p))
					{
						PRINT("merg2e " << pos1 << "," << j);
						
						out.push_back(e1[pos1]);
						return _merge(out, e2, j + 1, e1, pos1 + 1);
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
				
				_merge(e.vlist, vlist, 0, v.vlist, 0);
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
				ERRORMSG("& error");
			}
			return e;
		}
		bool intersect(const EDGE& v)
		{
			for (int i = 0; i < vlist.size(); i++)
			{
				for (int j = 0; j < v.vlist.size(); j++)
				{
					if (vlist[i].p == v.vlist[j].p)
					{
						return true;
					}
				}
			}
			return false;
		}
	};

	// ------------------------------------------
	extern std::vector<EDGE> gtable;
	EDGE gcuredge;
	// print
	inline void _PHGPRINT(const std::string& pre, const EDGE& e)
	{
		if (e.type == EDGE::VLIST)
		{
			estack.push_back(e.vlist);
			closeedge(estack.back());
		}

		gtable.push_back(e);
		gcuredge = e;
		if (e.type == EDGE::REAL)
		{
			PRINT(pre << e.fval);
		}
		else if (e.type == EDGE::QUAT)
		{
			quaternion q;
			q.fromangleaxis(e.q.w * PI / 180.0f, e.q.xyz().normcopy());
			PRINT(pre << q.x << "," << q.y << "," << q.z << "," << q.w);
		}
		for (auto& i : e.vlist)
			PRINTVEC3(i.p)
	}

	// ------------------------------------------
	#include "phg.hpp"
	// ------------------------------------------
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
				if (a.type == var::VLIST)
				{
					return a.invert();
				}
				return !a;
			}
		}
		default: {return 0; }
		}
	}

	// ------------------------------------------
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

// ------------------------------------------
// api
// ------------------------------------------
static var pushe(PMHG::code& cd, int args)
{
	//PRINTV(PHG_PARAM(1).vlist.size());
	if (args == 1)
	{
		if (PHG_PARAM(1).vlist.size() > 0)
		{
			estack.push_back(PHG_PARAM(1).vlist);
			closeedge(estack.back());
		}
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
	if (estack.empty())
		return 0;

	int n = 1;
	if (args == 1)
	{
		n = PHG_PARAM(1).fval;
	}
	for (int i = 0; i < n; i++)
		estack.pop_back();

	return 0;
}
static var extrudeedge(PMHG::code& cd, int args)
{
	if (args == 1)
	{
		float d = PHG_PARAM(1).fval;

		VECLIST& e1 = estack.back();
		vec norm = getedgenorm(e1);
		vec dv = norm * d;
		{
			for (int i = 0; i < e1.size(); i++)
			{
				e1[i].p += dv;
				e1[i].ind = -1;
			}
		}
	}
	else if (args == 2)
	{
		PMHG::EDGE e = PHG_PARAM(1);
		VECLIST& e1 = e.vlist;
		float d = PHG_PARAM(2).fval;
		vec norm = getedgenorm(e1);
		vec dv = norm * d;
		{
			for (int i = 0; i < e1.size(); i++)
			{
				e1[i].p += dv;
				e1[i].ind = -1;
			}
		}
		return e;
	}
	return INVALIDVAR;
}
static var moveedge(PMHG::code& cd, int args)
{
	if (args == 4)
	{
		PMHG::EDGE e = PHG_PARAM(1);
		float x = PHG_PARAM(2).fval;
		float y = PHG_PARAM(3).fval;
		float z = PHG_PARAM(4).fval;
		PRINTV(x);
		PRINTV(z);
		for (int i = 0; i < e.vlist.size(); i++)
		{
			e.vlist[i] = (e.vlist[i] + vec3(x, y, z));
		}
		return e;
	}
	else if(args == 3)
	{
		float x = PHG_PARAM(1).fval;
		float y = PHG_PARAM(2).fval;
		float z = PHG_PARAM(3).fval;

		VECLIST& e = estack.back();
		for (int i = 0; i < e.size(); i++)
		{
			e[i] = (e[i] + vec3(x, y, z));
		}
	}
	return INVALIDVAR;
}
static var scaleedge(PMHG::code& cd, int args)
{
	ASSERT (args == 1)
	
	float s = PHG_PARAM(1).fval;
	scaleedge(estack.back(), s);
	
	return INVALIDVAR;
}
static var yawedge(PMHG::code& cd, int args)
{
	float ang = PHG_PARAM(1).fval * PI / 180.0f;
	vec enorm = getedgenorm2(estack.back());
	rotedge(estack.back(), ang, enorm);
	if(coordstack.empty())
		coordstack.push_back(coord_t(estack.back()));
	coordstack.back().ux.rot(ang, enorm);
	coordstack.back().uz.rot(ang, enorm);
	return 0;
}
static var pitchedge(PMHG::code& cd, int args)
{
	float ang = PHG_PARAM(1).fval * PI / 180.0f;
	VECLIST& e = estack.back();
	vec ux, uy, uz;
	edgeax2(e, ux, uy, uz);
	if (weightlist.size() == e.size())
	{
		VECLIST oe;
		rotedge(e, ang, ux, oe);
		for (int i = 0; i < e.size(); i++)
		{
			e[i].p = blend(e[i].p, oe[i].p, weightlist[i]);
		}
	}
	else
	{
		rotedge(e, ang, ux);
	}
	if (coordstack.empty())
		coordstack.push_back(coord_t(estack.back()));
	coordstack.back().uy.rot(ang, ux);
	coordstack.back().uz.rot(ang, ux);
	return 0;
}
static var rolledge(PMHG::code& cd, int args)
{
	float ang = PHG_PARAM(1).fval * PI / 180.0f;
	VECLIST& e = estack.back();
	vec ux, uy, uz;
	edgeax2(e, ux, uy, uz);
	rotedge(e, ang, uz);
	if (coordstack.empty())
		coordstack.push_back(coord_t(estack.back()));
	coordstack.back().uy.rot(ang, uz);
	coordstack.back().ux.rot(ang, uz);
	return 0;
}
static var getcenter(PMHG::code& cd, int args)
{
	ASSERT(args == 1);
	vec o = getedgecenter(PHG_PARAM(1).vlist);
	PMHG::EDGE e;
	e.type = PMHG::EDGE::QUAT;
	e.q = quaternion(o.x, o.y, o.z, 0);

	return e;
}
inline var facepoly(VECLIST& e, vec3 ux = vec::UX)
{
	vec3 n = getedgenorm(e);
	if (n == ux)
		ux = vec::UZ;
	std::vector<vec3> tris;
	POLY::link_tri(e, ux, tris);
	//gcommonvertex = gsearchcomvertex = true;
	//gverindfindstart = gsubmesh->vertices.size();
	for (int i = 0; i < tris.size(); i += 3)
	{
		vertex p1(tris[i]);
		vertex p2(tris[i + 1]);
		vertex p3(tris[i + 2]);
		triang(p1, p2, p3);
	}
//	gsearchcomvertex = false;
	return INVALIDVAR;
}
static var facepoly(PMHG::code& cd, int args)
{
	if (estack.empty())
		return INVALIDVAR;

	std::vector<vec3> tris;
	VECLIST e = estack.back();
	vec3 n = getedgenorm(e);
	vec3 vx, vy; v2vxvy(n, vx, vy);
	POLY::link_tri(e, vx, tris);
	//gcommonvertex = true;
	//gverindfindstart = gsubmesh->vertices.size();
	for (int i = 0; i < tris.size(); i += 3)
	{
		vertex p1(tris[i]);
		vertex p2(tris[i + 1]);
		vertex p3(tris[i + 2]);
		triang(p1, p2, p3);
	}
	//gcommonvertex = false;
	return INVALIDVAR;
}
inline void addpoints(crvec p1, crvec p2, VECLIST& e)
{
	int len = (p2 - p1).len()-1;
	vec3 u = (p2 - p1); u.norm();
	if(e.empty() || e.back().p != p1)
		e.push_back(p1);
	
	for (int i = 1; i < len; i++)
	{
		e.push_back(p1 + u * i);
	}
	e.push_back(p2);
}
inline void face_2e_hole(PMHG::EDGE& e1, PMHG::EDGE& e2, PMHG::EDGE& hole)
{
	closeedge(hole.vlist);
	int len = int(_MIN(e1.vlist.size(), e2.vlist.size())-1);
	for (int i = 0; i < len; i++)
	{
		PMHG::EDGE ee;
		addpoints(e1.vlist[i], e1.vlist[i + 1], ee.vlist);
		addpoints(e1.vlist[i + 1], e2.vlist[i + 1], ee.vlist);
		addpoints(e2.vlist[i + 1], e2.vlist[i], ee.vlist);
		addpoints(e2.vlist[i], e1.vlist[i], ee.vlist);

		if(ee.intersect(hole))
		{
			PMHG::EDGE eee;
			{
				eee = ee | (hole);
				PMHG::gtable.push_back(eee);
				facepoly(eee.vlist);
			}
			phaseedge(ee.vlist, ee.vlist.size() / 2);
			{
				eee = ee | (hole);
				PMHG::gtable.push_back(eee);
				facepoly(eee.vlist);
			}
		}
		else
		{
			plane(e1.vlist[i], e1.vlist[i + 1], e2.vlist[i + 1], e2.vlist[i]);
		}
	}
}
void cutedge(crvec v, VECLIST& e, VECLIST& eo)
{
	int i = 1;
	eo.push_back(e[0]);
	for (; i < e.size(); i++)
	{
		vec3 nv = (e[i] - e[i - 1]);
		if (v.dot(nv) <= 1e-4)
			break;

		eo.push_back(e[i]);
	}
	e.assign(e.begin() + i-1, e.end());
}
void faceA(VECLIST& e1, VECLIST& e2)
{
	while (e1.size() >= 2)
	{
		vec3 v = e1[1] - e1[0];
		VECLIST ee;
		cutedge(v, e1, ee);
		VECLIST ee2;
		cutedge(v, e2, ee2);

		//PRINTV(ee.size());
		linkedge(ee, ee2, true);
		closeedge(ee);
		facepoly(ee, v);
	}
}
static var face(PMHG::code& cd, int args)
{
	if (args == 3)
	{
		face_2e_hole(PHG_PARAM(1), PHG_PARAM(2), PHG_PARAM(3));
	}
	else if (args == 2)
		faceA(PHG_PARAM(1).vlist, PHG_PARAM(2).vlist);
	//face_noclose(PHG_PARAM(1).vlist, PHG_PARAM(2).vlist);
	else if (args == 1)
	{
		facepoly(PHG_PARAM(1).vlist);
	}
	else if (args == 0)
		face(estack[estack.size() - 2], estack.back());

	return 0;
}

//------------------------------------------
static void initphg()
{
	PRINT("setup PMHG");
	
	reset();

	PMHG::register_api("push", pushe);
	PMHG::register_api("pop", pope);

	PMHG::register_api("ext", extrudeedge);
	PMHG::register_api("mov", moveedge);
	PMHG::register_api("scl", scaleedge);

	PMHG::register_api("yaw", yawedge);
	PMHG::register_api("pit", pitchedge);
	PMHG::register_api("rol", rolledge);

	PMHG::register_api("cent", getcenter);

	PMHG::register_api("face", face);
	PMHG::register_api("facepoly", facepoly);
}

//------------------------------------------
// VB
//------------------------------------------
extern "C"
{
	 int EXPORT_API _stdcall VB_dopmhg(const char* script)
	{
		std::string str(script);
		PRINTV(str);

		if (str.find(".e") != std::string::npos)
		{
			if(PMHG::table == 0)
			{
				PMHG::setup();
				renderstate = 0;
			}
			resetsm();
			estack.clear();
			PMHG::dofile(str.c_str());
		}
		else
			PMHG::dostring(str.c_str());
		return 0;
	}
	EXPORT_API float _stdcall VB_curval()
	{
		return PMHG::gcuredge.fval;
	}
	EXPORT_API float _stdcall VB_setcur(int index)
	{
		PRINT(index);
		PMHG::gcuredge = PMHG::gtable[index];
		return PMHG::gcuredge.fval;
	}
	EXPORT_API int _stdcall VB_tablecnt()
	{
		return PMHG::gtable.size();
	}
}

// =============================================
// test
// =============================================
void test()
{
	{// zero element
		PMHG::gtable.clear();
		PMHG::EDGE e; e.vlist.push_back(vec3::ZERO);
		PMHG::gtable.push_back(e);
	}

	PMHG::dofile("main.e");
}
