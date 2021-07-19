#undef var
#undef INVALIDVAR

#define var			ELEMENT
#define INVALIDVAR	ELEMENT(0)

#undef PHGPRINT
#define PHGPRINT	GROUP::_PHGPRINT

#define PHG_VAR(name, defaultval) (GROUP::gvarmapstack.stack.empty() || GROUP::gvarmapstack.stack.front().find(#name) == GROUP::gvarmapstack.stack.front().end() ? defaultval : GROUP::gvarmapstack.stack.front()[#name])
#define PHG_PARAM(index)	cd.valstack.get(args - index)

#define DEFAULT_ELEMENT	\
	ELEMENT() {}\
	ELEMENT(int _val) {}\
	operator const int& (){return 1;}\
	bool operator == (const ELEMENT& v) const{return false;}
