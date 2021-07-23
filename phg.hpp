/****************************************************************************
				Phg2.0
				脚本是群论的扩展
语法示例:	

'function					
#blend(a, b, alpha)
{
	$ a*(1-alpha) + b*alpha;
}

'call function
ab = blend(2,8, 0.25)
>ab;

'if
?(i = 1){
t = t + 1;
}:{
t = t - 1;
}
>t;

'calc
yy = 1*2+ 4 * 8;
> yy;

'loop
@(yy < 100){
yy = yy + 1;
}
> yy;

****************************************************************************/

//#define PHG_VAR(name, defaultval) (PHG::gcode.varmapstack.stack.empty() || PHG::gcode.varmapstack.stack.front().find(#name) == PHG::gcode.varmapstack.stack.front().end() ? defaultval : PHG::gcode.varmapstack.stack.front()[#name])
//#define PHG_PARAM(index)	cd.valstack.get(args - index)

//#define var			real
//#define INVALIDVAR	(0)

// ----------------------------------------------------------------------
// PHG
// ----------------------------------------------------------------------
//namespace PHG{
#define INVALIDFUN	cd.funcnamemap.end()

#define varname		string
#define toint(v)	(int)(v)

#define opr			char
#define funcname	string
#define functionptr	const char*

#define NAME		0x01FF
#define NUMBER		0x02FF
#define OPR			0x03FF
#define LGOPR		0x04FF
	
extern struct code;
extern var callfunc(code& cd);

typedef void (*parser_fun)(code& cd);
static parser_fun parser = 0;
typedef void(*statement_fun)(code& cd);
static statement_fun statement = 0;

static char rank[256];

std::vector<var> gtable;

extern struct varmapstack_t;
extern varmapstack_t	gvarmapstack;

inline int add2table(const var& v)
{
	for (int i = 0; i < gtable.size(); i++)
	{
		if (gtable[i] == v)
		{
			//PRINT("add2table same! i=" << i);
			return i;
		}
	}

	gtable.push_back(v);
	return gtable.size() - 1;
}

// API
typedef var (*fun_t)(code& cd, int stackpos);
struct api_fun_t
{
	int args = 0;
	fun_t fun;
};
	
std::map<string, api_fun_t> api_list;

void(*table)(code& cd);

// 运算
var(*act)(code& cd, int args);
	
// -----------------------------------------------------------------------
inline bool checkline(char c) {
	return (c == '\n');
}
inline bool checkspace(char c) {
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}
inline bool iscalc(char c) {
	return c == '+' || c == '-' || c == '*' || c == '/' || c == '!' || c == '&' || c == '|';
}
inline bool islogic(char c) {
	return c == '>' || c == '<' || c == '=' || c == '&' || c == '|' || c == '^';
}
inline bool isname(char c) {
	return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
}
inline bool isnum(char c) {
	return c >= '0' && c <= '9';
}
inline bool isbrackets(char c) {
	return c == '(';
}

// stacks define
static struct codestack_t
{
	const char* buff[2048];
	int top;
	void push(const char* c) {
		ASSERT(top + 1 < 2048);
		buff[++top] = c;
	}
	const char* pop() {
		ASSERT(top > -1);
		return buff[top--];
	}
	const char* cur() {
		ASSERT(top != -1);
		return buff[top];
	}
	bool empty() {
		return top == -1;
	}
	codestack_t() {
		top = -1;
	}
};

static struct valstack_t
{
	var buff[2048];
	int top;
	void push(const var& v) {
		//PRINT("valstack PUSH " << top);
		ASSERT(top + 1 < 2048);
		buff[++top] = v;
	}
	var& pop() {
		//PRINT("valstack POP " << top);
		ASSERT(top > -1);
		return buff[top--];
	}
	var& cur() {
		ASSERT(top != -1);
		return buff[top];
	}
	var& get(int pos) {
		ASSERT(top != -1);
		ASSERT(top - pos >= 0);
		return buff[top - pos];
	}
	void reset()
	{
		top = -1;
	}
	valstack_t() {
		top = -1;
	}
};

static struct oprstack_t
{
	opr buff[2048];
	int top;
	void push(opr c) {
		//PRINT("oprstack:PUSH " << c)
		ASSERT(top + 1 < 2048);
		buff[++top] = c;
	}
	opr pop() {
		//PRINT("oprstack:POP")
		ASSERT(top > -1);
		return buff[top--];
	}
	opr cur() {
		ASSERT(top != -1);
		return buff[top];
	}
	void setcur(opr o) {
		ASSERT(top != -1);
		buff[top] = o;
	}
	bool empty() {
		return top == -1;
	}
	oprstack_t() {
		top = -1;
	}
};

static struct varmapstack_t
{
	using varmap_t = std::map<varname, var>;
	std::vector<varmap_t> stack;
		
	void push()
	{
		//PRINT("varmapstack PUSH");
		stack.push_back(varmap_t());
	}
	void pop()
	{
		//PRINT("varmapstack POP");
		stack.pop_back();
	}
	void addvar(const char* name, const var& v)
	{
		PRINT("addvar: " << name);// << " = " << v);
		//if(!gtable.empty())
		//	add2table(v);

		if (stack.empty())
			push();

		stack.back()[name] = v;
	}
	var getvar(const char* name)
	{
		PRINT("getvar = " << name)
		if (stack.empty())
			return INVALIDVAR;

		for (int i = stack.size() - 1; i >= 0; i--)
		{
			varmap_t& varlist = stack[i];
			if (varlist.find(name) != varlist.end())
			{
				return varlist[name];
			}
		}

		return INVALIDVAR;
	}
	void clear()
	{
		stack.clear();
	}
} gvarmapstack;

// -----------------------------------------------------------------------
// code

static struct code
{
	const char* ptr;
	std::map<funcname, functionptr>	funcnamemap;		
	//varmapstack_t	varmapstack;

	codestack_t		codestack;
	valstack_t		valstack;
	oprstack_t		oprstack;

	code() {}
	code(const char* buf) {
		ptr = buf;
		//varmapstack.clear();
		funcnamemap.clear();
	}
	char next() {
		while (!eoc(++ptr) && checkspace(*(ptr)));
		return (*ptr);
	}
	char next2() {
		while (!eoc(++ptr)){
			if (!checkspace(*(ptr)) && !isname(*(ptr)))
				break;
		}
		return (*ptr);
	}
	char next3() {
		while (!eoc(++ptr)) {
			if (!checkspace(*(ptr)) && !isname(*(ptr)) && !isnum(*(ptr)))
				break;
		}
		return (*ptr);
	}
	char nextline() {
		while (!eoc(++ptr) && !checkline(*(ptr)));
		return (*++ptr);
	}
	char getnext() {
		const char* p = ptr;
		while (!eoc(++p) && checkspace(*(p)));
		return (*p);
	}
	char getnext2() {
		const char* p = ptr;
		while (!eoc(++p)){
			if(!checkspace(*(p)) && (!isname(*(p))))
				break;
		}
		return (*p);
	}
	char getnext3() {
		const char* p = ptr;
		while (!eoc(++p)) {
			if (!checkspace(*(p)) && '.' != (*(p)) && !isnum(*(p)))
				break;
		}
		return (*p);
	}
	char getnext4() {
		const char* p = ptr;
		while (!eoc(++p)) {
			if (!checkspace(*(p)) && (!isname(*(p)) && !isnum(*(p))))
				break;
		}
		return (*p);
	}
	bool eoc(const char* p = 0) {
		p == 0 ? p = ptr : 0;
		return (*p) == '\0';
	}
	char cur() {
		return *ptr;
	}
	const char* getname() {
		static char buf[128];
		char* pbuf = buf;
		const char* p = ptr;
		if (!eoc(p) && !isnum(*p))
		{
			while (!eoc(p) && !checkspace(*p) && (isname(*p) || isnum(*p)))
				*(pbuf++) = *(p++);
		}
		(*pbuf) = '\0';
		return buf;
	}
};
	
// get char
static short get(code& cd)
{
	for (; !cd.eoc(); cd.next()) {
		char c = cd.cur();
		if (checkspace(c))
			continue;
		else if (isdigit(c)) {
			return NUMBER;
		}
		else if (iscalc(c)) {
			return OPR;
		}
		else if (islogic(c)) {
			return LGOPR;
		}
		else if (isname(c)) {
			return NAME;
		}
		else
			return c;
	}
	return 0;
}
static short gettype(char c)
{
	if (isdigit(c)) {
		return NUMBER;
	}
	else if (iscalc(c)) {
		return OPR;
	}
	else if (islogic(c)) {
		return LGOPR;
	}
	else if (isname(c)) {
		return NAME;
	}
	else
		return c;
}
/*
// 运算
static var act_default(code& cd, int args)
{
	opr o = cd.oprstack.pop();

	PHGPRINT("act:" << o << " args = " << args)

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
		return a > b;
	}
	case '<': {
		var b = cd.valstack.pop();
		var a = cd.valstack.pop();
		return a < b;
	}
	case '=': {
		var b = cd.valstack.pop();
		var a = cd.valstack.pop();
		return a == b;
	}
	case '&': {
		var b = cd.valstack.pop();
		var a = cd.valstack.pop();
		return a && b;
	}
	case '|': {
		var b = cd.valstack.pop();
		var a = cd.valstack.pop();
		return a || b;
	}
	case '!': {
		var a = cd.valstack.pop();
		return !a;
	}
	default: {return 0; }
	}
}
*/
static inline var chars2var(code& cd) {
	static char buff[128];
	bool isreal = false;
	int i = 0;
	for (; i < 128; i++) {
		char c = cd.cur();
		if (c == '.')
			isreal = true;
		if (!isdigit(c) && c != '.')
			break;
		buff[i] = c;
		cd.next();
	}
	buff[i] = '\0';

	if (!isreal && !gtable.empty())
	{
		int number = atoi(buff);
		if (number < 0 || number >= gtable.size())
		{
			ERRORMSG("chars2var error! number=" << number);
			return INVALIDVAR;
		}
		//PRINTV(number);
		return gtable[number];
	}
	//PRINT("chars2var isreal=" << isreal << " " << buff);
	return isreal ? (real)atof(buff) : atoi(buff);
}

// get value
static void getval(code& cd, short type) {
		
	if (type == NUMBER) {
		cd.valstack.push(chars2var(cd));
		/*if (cd.oprstack.empty() || !(iscalc(cd.oprstack.cur()) || islogic(cd.oprstack.cur()))) {
			cd.oprstack.push('.');
		}*/
	}
	else if (type == NAME) {
		if (api_list.find(cd.getname()) != api_list.end() ||
			cd.funcnamemap.find(cd.getname()) != INVALIDFUN) {
			cd.valstack.push(callfunc(cd));
		}
		else {
			cd.valstack.push(gvarmapstack.getvar(cd.getname()));
			cd.next3();
		}
		//if (cd.oprstack.empty() || !(iscalc(cd.oprstack.cur()) || islogic(cd.oprstack.cur()))) {
		//	cd.oprstack.push('.');
		//}
	}
}
// finished trunk
static void finishtrunk(code& cd, int trunkcnt = 0, char sk = '{', char ek = '}')
{
	while (!cd.eoc()) {
		char c = cd.cur();
		if (c == sk) {
			trunkcnt++;
		}
		else if (c == ek) {
			trunkcnt--;

			if (trunkcnt == 0) {
				cd.next();
				break;
			}
		}
		cd.next();
	}
}

// -----------------------------------------------------------------------
// 表达式 for example: x=a+b, v = fun(x), x > 2 || x < 5
static var expr(code& cd, byte args0 = 0, byte rank0 = 0)
{
	//PRINT("expr(");
	int args = args0;
	int oprs = 0;
	while (!cd.eoc()) {
		short type = get(cd);
		if (type == NAME || type == NUMBER) {
			getval(cd, type);
			args++;
		}
		else if (type == OPR) {
			opr o = cd.cur();
			
			if (!cd.oprstack.empty() && cd.oprstack.cur() == '.')
				cd.oprstack.setcur(o);
			else
			{
				cd.oprstack.push(o);
				oprs++;
			}

			cd.next();

			if (iscalc(cd.cur())) {
				cd.valstack.push(expr(cd));
				args++;
			}
			else {
				if (cd.cur() == '(')
				{
					cd.next();
					cd.valstack.push(expr(cd));
					args++;
				}
				char no = cd.getnext3();
				if (cd.cur() != '(' && 
					iscalc(no))
				{
					if (cd.cur() == ')')
						cd.next();

					type = get(cd);

					if (rank[o] >= rank[no]) {
						getval(cd, type);
						args++;
						if (rank0 >= rank[o])
						{
							//PRINT("+++++++++++ " << o);
							return act(cd, args);
						}
						cd.valstack.push(act(cd, args));
						args = 1;
					}
					else {
						getval(cd, type);
						cd.valstack.push(expr(cd, 1, rank[no]));
						args++;
						//PRINT("---------- " << args);
						if (args >= 2)
						{
							cd.valstack.push(act(cd, args));
							args = 1;
						}
					}
				}
			}
		}
		else if (type == LGOPR) {
			//if (cd.oprstack.cur() == '.')
			//	cd.oprstack.setcur(cd.cur());
			//else
			{
				cd.oprstack.push(cd.cur());
				oprs++;
			}
			cd.next();
		}
		else {
			char c = cd.cur();
			if (c == '(') {
				cd.next();
				var v = expr(cd);
				cd.next();
				cd.valstack.push(v);
				
				args++;
			}
			else if (c == ')' || c == ']' || c == ';' || c == ',' || c == '{' || c == '\n' || c == '\r') {

				if (!cd.oprstack.empty() &&
					(iscalc(cd.oprstack.cur()) || islogic(cd.oprstack.cur())) &&
					oprs > 0)
				{
					//const var& ret = act(cd, args);PRINT(")");return ret;
					//PRINTV(oprs);
					return act(cd, args);
				}
				else {
					//const var& ret = cd.valstack.pop();PRINT(")"); return ret;
					return cd.valstack.pop();
				}
			}
		}
	}
	ERRORMSG("';' is missing?");
	return INVALIDVAR;
}
// single var
static void singvar(code& cd) {
	string name = cd.getname();
	PRINT("singvar: " << name);
	cd.next3();
	ASSERT(cd.cur() == '=');
	cd.next();

	var v = expr(cd);
	cd.next();
	gvarmapstack.addvar(name.c_str(), v);
}

// statement
static void statement_default(code& cd) {
		
	short type = get(cd);
	if (type == NAME) {
		char nc = cd.getnext4();
		if (nc == '=') {
			singvar(cd);
		}
		else if (nc == '(') {
			callfunc(cd);
			cd.next();
		}
	}
	else if (cd.cur() == '>') {
		cd.next();
		const var& v = expr(cd);
		PHGPRINT("> ", v);
		cd.next();
	}
	else
	{
		cd.next();
	}
}

// subtrunk
static int subtrunk(code& cd, var& ret)
{
	while (!cd.eoc()) {
		short type = get(cd);
		//PRINTV(cd.cur());
		if (cd.cur() == '{') 
		{
			cd.next();
			int rettype = subtrunk(cd, ret);
			if (rettype > 0)
				return rettype;
		}
		else if (cd.cur() == '}')
		{
			cd.next();
			break;
		}
		else if (type == '\'')
		{
			cd.nextline();
		}
		else if (type == '?') 
		{
			ASSERT(cd.next() == '(')
				cd.next();
			const var& e = expr(cd);
			cd.next();

			if (e == 0) {
				finishtrunk(cd, 0, '{', '}');
				if (cd.cur() == ':')
					cd.next();
				else
					continue;
			}
			cd.next();

			int rettype = subtrunk(cd, ret);
			if (rettype == 1)
			{
				finishtrunk(cd, 1);
				return rettype;
			}
			if (rettype == 2) {
				return rettype;
			}

		}
		else if (type == '@')
		{
			if (cd.next() == '(') {
				cd.next();

				const char* cp = cd.ptr;
			codepos1:
				var e = expr(cd);
				cd.next();
			
				if (e != 0) {
					cd.next();
					int rettype = subtrunk(cd, ret);
					if (rettype == 1)
					{
						finishtrunk(cd, 1);
						break;
					}
					if (rettype == 2) {
						return rettype;
					}

					cd.ptr = cp;
					PRINT("loop ");
					goto codepos1;
				}
				else {
					finishtrunk(cd, 0);
				}
			}
			else 
			{
				int loopcnt = toint(expr(cd));
				cd.next();
				const char* cp = cd.ptr;
				for (int i = 1; i <= loopcnt; i++) {
					cd.ptr = cp;
					int rettype = subtrunk(cd, ret);
					
					if (rettype == 1)
					{
						finishtrunk(cd, 1);
						break;
					}
					if (rettype == 2) {
						return rettype;
					}
					
					PRINT("loop " << i);
				}
			}
		}
		else if (type == '$')
		{
			cd.next();
			if (cd.cur() == '@') {
				return 1;
			}
			else {
				ret = expr(cd);
				return 2;
			}
		}
		else
		{
			statement(cd);
		}
	}
	return 0;
}
	
// 函数
static var callfunc_phg(code& cd) {
	funcname fnm = cd.getname();
	PRINT("callfunc: " << fnm << "()");
	PRINT("{");
	ASSERT(cd.next3() == '(');

	cd.next();
	while (!cd.eoc()) {
		char c = cd.cur();
		if (c == ')') {
			cd.next();
			break;
		}
		else if (c == ',') {
			cd.next();
			continue;
		}
		else {
			var e = expr(cd);
			//PRINTV(e);
			cd.valstack.push(e);
		}
	}

	cd.codestack.push(cd.ptr);

	if (api_list.find(cd.getname()) != api_list.end() || 
		cd.funcnamemap.find(fnm) == INVALIDFUN)
	{
		ERRORMSG("No function named: '" << fnm << "'")
		return INVALIDVAR;
	}
	cd.ptr = cd.funcnamemap[fnm];

	cd.next3();
	ASSERT(cd.cur() == '(');

	gvarmapstack.push();
	cd.next();
	std::vector<std::string> paramnamelist;
	std::vector<var> paramvallist;
	while (!cd.eoc()) {
		char c = cd.cur();
		if (c == ')') {
			cd.next();
			break;
		}
		else if (c == ',') {
			cd.next();
		}
		else {
			paramnamelist.push_back(cd.getname());
			paramvallist.push_back(cd.valstack.pop());
			cd.next2();
		}
	}

	for(int i = 0; i < paramnamelist.size(); i ++)
	{
		gvarmapstack.addvar(paramnamelist[i].c_str(), paramvallist[paramvallist.size() - 1 - i]);
	}

	var ret = INVALIDVAR;
	ASSERT(subtrunk(cd, ret) == 2);
	gvarmapstack.pop();

	cd.ptr = cd.codestack.pop();
	PRINT("return ");
	PRINT("}");
	return ret;
}

static var callfunc(code& cd) {
	funcname fnm = cd.getname();
	if (api_list.find(fnm) != api_list.end())
	{
		PRINT("API:" << fnm);
		api_fun_t& apifun = api_list[fnm];
		apifun.args = 0;

		ASSERT(cd.next2() == '(');

		cd.next();
		while (!cd.eoc()) {
			char c = cd.cur();
			
			if (c == ')') {
				cd.next();
				break;
			}
			else if (c == ',') {
				cd.next();
				continue;
			}
			else {
				var e = expr(cd);
				//PRINTV(e);
				cd.valstack.push(e);
				apifun.args++;
			}
		}
		var ret = apifun.fun(cd, apifun.args);
		for (int i = 0; i < apifun.args; i++)
			cd.valstack.pop();
		return ret;
	}
	else
		return callfunc_phg(cd);
}

// func
static void func(code& cd) {
	funcname fnm = cd.getname();
	PRINT("define func: " << fnm);
	ASSERT(cd.funcnamemap[fnm] == 0);
	cd.funcnamemap[fnm] = cd.ptr;
	cd.next();
	finishtrunk(cd, 0);
}

// parser
static void parser_default(code& cd) {
	PRINT("--------PHG---------");
	PRINT(cd.ptr);
	PRINT("--------------------");
	
	rank['|'] = 1;
	rank['^'] = 1;
	rank['&'] = 2;
	rank['+'] = 3;
	rank['-'] = 3;
	rank['*'] = 4;
	rank['/'] = 4;
	rank['!'] = 5;

	//getchar();

	PRINTV(gvarmapstack.stack.size());
	gvarmapstack.push();

	while (!cd.eoc()) {
		short type = get(cd);

		//PRINTV(cd.cur());

		if (type == '\''){
			cd.nextline();
		}
		else if (type == '#') {
			cd.next();
			func(cd);
		}
		else if (type == '[') {
			table(cd);
		}
		else {
			var ret = INVALIDVAR;
			subtrunk(cd, ret);
		}
	}
}

// ------------------------------------------
static code gcode;
// ------------------------------------------
void init()
{
	if (!parser)
		parser = parser_default;
	if (!statement)
		statement = statement_default;
}

// dofile
inline void dofile(const char* filename)
{
	init();

	FILE* f = fopen(filename, "rb");
	ASSERT(f != 0);
	char buf[1024];
	int sp = ftell(f);
	fseek(f, 0, SEEK_END);
	int sz = ftell(f) - sp;
	fseek(f, 0, SEEK_SET);
	fread(buf, 1, sz, f);
	buf[sz] = '\0';
	fclose(f);

	(*parser)(gcode = code(buf));
	PRINT("\n");
}

// dostring
inline void dostring(const char* str)
{
	init();

	parser(gcode = code(str));
	PRINT("\n");
}

// API
inline void register_api(crstr name, fun_t fun)
{
	api_list[name].fun = fun;
}
//}
