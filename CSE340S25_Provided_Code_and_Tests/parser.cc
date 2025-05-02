#include <iostream>
#include <map>
#include <vector>
#include <string>
#include "lexer.h"
#include "execute.h"
#include <set>

using namespace std;

LexicalAnalyzer lexer;
map<string, int> variable_table;
int mem_index = 0;
std::set<int> seen_cases;

int get_or_add_var_location(string name) {
    if (variable_table.find(name) == variable_table.end()) {
        variable_table[name] = mem_index++;
    }
    return variable_table[name];
}

int store_constant(int value) {
    mem[mem_index] = value;
    return mem_index++;
}

void syntax_error(){
    cout << "SYNTAX ERROR !!!\n";
    exit(0);
}

Token expect(TokenType expected_type)
{
    Token t = lexer.GetToken();
    //cout << "consuming "; t.Print(); cout << "\n";
    if (t.token_type != expected_type){
        //cout << "error on line: " << t.line_no << "\n";
        syntax_error();
    }
    return t;
}

InstructionNode* parse_stmt();
InstructionNode* parse_if_stmt();
InstructionNode* parse_assign_stmt();
InstructionNode* parse_input_stmt();
InstructionNode* parse_output_stmt();
InstructionNode* parse_stmt_list();
InstructionNode* parse_body();
InstructionNode* parse_while_stmt();
InstructionNode* parse_for_stmt();
InstructionNode* parse_switch_stmt();

InstructionNode* parseDefaultCase();
InstructionNode* parseCase(Token, struct InstructionNode*);
InstructionNode* parseCaseList(Token, struct InstructionNode*);
InstructionNode* parseSwitchStmt();

void parse_var_section();
void parse_inputs();

InstructionNode* parse_Generate_Intermediate_Representation() {
    parse_var_section();
    InstructionNode* body = parse_body();
    parse_inputs();
    return body;
}

void parse_var_section() {
    Token t = lexer.GetToken();
    if (t.token_type != ID) {
        cout << "Syntax error: Expected ID in var section\n";
        exit(1);
    }

    while (true) {
        get_or_add_var_location(t.lexeme);
        t = lexer.GetToken();
        if (t.token_type == COMMA) {
            t = lexer.GetToken();
            if (t.token_type != ID) {
                cout << "Syntax error after comma\n";
                exit(1);
            }
        } else if (t.token_type == SEMICOLON) {
            break;
        } else {
            cout << "Syntax error in var section\n";
            exit(1);
        }
    }
}

InstructionNode* parse_body() {
    Token t = lexer.GetToken(); // LBRACE
    if (t.token_type != LBRACE) {
        cout << "Syntax error: Expected {\n";
        exit(1);
    }

    InstructionNode* list = parse_stmt_list();

    t = lexer.GetToken(); // RBRACE
    if (t.token_type != RBRACE) {
        cout << "Syntax error: Expected }\n";
        exit(1);
    }

    return list;
}

InstructionNode* parse_stmt_list() {
    InstructionNode* head = parse_stmt();
    InstructionNode* tail = head;

    // Traverse to the last node
    while (tail->next != nullptr) {
        tail = tail->next;
    }

    Token t = lexer.peek(1);
    if (t.token_type == ID || t.token_type == WHILE || t.token_type == IF ||
        t.token_type == INPUT || t.token_type == OUTPUT ||
        t.token_type == FOR || t.token_type == SWITCH) {

        InstructionNode* next = parse_stmt_list();
        tail->next = next;
    }

    return head;
}

InstructionNode* parse_stmt() {
    Token t = lexer.peek(1);

    if (t.token_type == ID) return parse_assign_stmt();
    if (t.token_type == INPUT) return parse_input_stmt();
    if (t.token_type == OUTPUT) return parse_output_stmt();
    if (t.token_type == IF) return parse_if_stmt();
    if (t.token_type == WHILE) return parse_while_stmt();
    if (t.token_type == FOR) return parse_for_stmt();
    if (t.token_type == SWITCH) return parseSwitchStmt();



    cout << "Syntax error: Unknown stmt start token\n";
    exit(1);
    return nullptr;
}

InstructionNode* parse_assign_stmt() {
    InstructionNode* node = new InstructionNode;
    node->type = ASSIGN;
    node->next = nullptr;

    Token id = lexer.GetToken(); // ID
    lexer.GetToken(); // EQUAL

    Token rhs1 = lexer.GetToken(); // first operand
    Token op = lexer.peek(1);

    if (op.token_type == PLUS || op.token_type == MINUS ||
        op.token_type == MULT || op.token_type == DIV) {
        lexer.GetToken(); // consume operator
        Token rhs2 = lexer.GetToken();

        node->assign_inst.op = (op.token_type == PLUS) ? OPERATOR_PLUS :
                               (op.token_type == MINUS) ? OPERATOR_MINUS :
                               (op.token_type == MULT) ? OPERATOR_MULT :
                               OPERATOR_DIV;

        node->assign_inst.op1_loc = (rhs1.token_type == ID) ?
                                     get_or_add_var_location(rhs1.lexeme) :
                                     store_constant(stoi(rhs1.lexeme));

        node->assign_inst.op2_loc = (rhs2.token_type == ID) ?
                                     get_or_add_var_location(rhs2.lexeme) :
                                     store_constant(stoi(rhs2.lexeme));
    } else {
        node->assign_inst.op = OPERATOR_NONE;
        node->assign_inst.op1_loc = (rhs1.token_type == ID) ?
                                     get_or_add_var_location(rhs1.lexeme) :
                                     store_constant(stoi(rhs1.lexeme));
        node->assign_inst.op2_loc = -1;
    }

    node->assign_inst.lhs_loc = get_or_add_var_location(id.lexeme);

    Token end = lexer.GetToken(); // SEMICOLON
    if (end.token_type != SEMICOLON) {
        cout << "Syntax error: Expected ;\n";
        exit(1);
    }

    return node;
}

InstructionNode* parse_input_stmt() {
    lexer.GetToken(); // INPUT
    Token id = lexer.GetToken(); // ID
    lexer.GetToken(); // SEMICOLON

    InstructionNode* node = new InstructionNode;
    node->type = IN;
    node->next = nullptr;
    node->input_inst.var_loc = get_or_add_var_location(id.lexeme);
    return node;
}

InstructionNode* parse_output_stmt() {
    lexer.GetToken(); // OUTPUT
    Token id = lexer.GetToken(); // ID
    lexer.GetToken(); // SEMICOLON

    InstructionNode* node = new InstructionNode;
    node->type = OUT;
    node->next = nullptr;
    node->output_inst.var_loc = get_or_add_var_location(id.lexeme);
    return node;
}

InstructionNode* parse_if_stmt() {
    lexer.GetToken(); // IF

    Token op1 = lexer.GetToken(); // primary
    Token relop = lexer.GetToken(); // > < !=
    Token op2 = lexer.GetToken(); // primary

    ConditionalOperatorType cond_op;
    if (relop.token_type == GREATER) cond_op = CONDITION_GREATER;
    else if (relop.token_type == LESS) cond_op = CONDITION_LESS;
    else cond_op = CONDITION_NOTEQUAL;

    int op1_loc = (op1.token_type == ID) ? get_or_add_var_location(op1.lexeme) : store_constant(stoi(op1.lexeme));
    int op2_loc = (op2.token_type == ID) ? get_or_add_var_location(op2.lexeme) : store_constant(stoi(op2.lexeme));

    InstructionNode* cjmp = new InstructionNode;
    cjmp->type = CJMP;
    cjmp->next = nullptr;
    cjmp->cjmp_inst.condition_op = cond_op;
    cjmp->cjmp_inst.op1_loc = op1_loc;
    cjmp->cjmp_inst.op2_loc = op2_loc;

    InstructionNode* body = parse_body();
    cjmp->next = body;

    InstructionNode* temp = body;
    while (temp->next != nullptr)
        temp = temp->next;

    InstructionNode* noop = new InstructionNode;
    noop->type = NOOP;
    noop->next = nullptr;

    temp->next = noop;
    cjmp->cjmp_inst.target = noop;

    return cjmp;
}

InstructionNode* parse_while_stmt() {
    lexer.GetToken();

    Token op1 = lexer.GetToken();
    Token relop = lexer.GetToken();
    Token op2 = lexer.GetToken();

    ConditionalOperatorType cond_op;
    if (relop.token_type == GREATER) cond_op = CONDITION_GREATER;
    else if (relop.token_type == LESS) cond_op = CONDITION_LESS;
    else cond_op = CONDITION_NOTEQUAL;

    int op1_loc = (op1.token_type == ID) ? get_or_add_var_location(op1.lexeme) : store_constant(stoi(op1.lexeme));
    int op2_loc = (op2.token_type == ID) ? get_or_add_var_location(op2.lexeme) : store_constant(stoi(op2.lexeme));

    InstructionNode* cjmp = new InstructionNode;
    cjmp->type = CJMP;
    cjmp->next = nullptr;
    cjmp->cjmp_inst.condition_op = cond_op;
    cjmp->cjmp_inst.op1_loc = op1_loc;
    cjmp->cjmp_inst.op2_loc = op2_loc;

    InstructionNode* body = parse_body();
    cjmp->next = body;

    InstructionNode* temp = body;
    while (temp->next != nullptr) {
        temp = temp->next;
    }

    InstructionNode* jmp = new InstructionNode;
    jmp->type = JMP;
    jmp->next = nullptr;
    jmp->jmp_inst.target = cjmp;

    temp->next = jmp;

    InstructionNode* noop = new InstructionNode;
    noop->type = NOOP;
    noop->next = nullptr;

    jmp->next = noop;
    cjmp->cjmp_inst.target = noop;

    return cjmp;
}

InstructionNode* parse_for_stmt() {
    lexer.GetToken(); 
    lexer.GetToken();

    InstructionNode* assign1 = parse_assign_stmt();

    Token op1 = lexer.GetToken();
    Token relop = lexer.GetToken();
    Token op2 = lexer.GetToken();
    Token semi = lexer.GetToken(); 

    ConditionalOperatorType cond_op;
    if (relop.token_type == GREATER) cond_op = CONDITION_GREATER;
    else if (relop.token_type == LESS) cond_op = CONDITION_LESS;
    else cond_op = CONDITION_NOTEQUAL;

    int op1_loc = (op1.token_type == ID) ? get_or_add_var_location(op1.lexeme) : store_constant(stoi(op1.lexeme));
    int op2_loc = (op2.token_type == ID) ? get_or_add_var_location(op2.lexeme) : store_constant(stoi(op2.lexeme));

    InstructionNode* cjmp = new InstructionNode;
    cjmp->type = CJMP;
    cjmp->next = nullptr;
    cjmp->cjmp_inst.condition_op = cond_op;
    cjmp->cjmp_inst.op1_loc = op1_loc;
    cjmp->cjmp_inst.op2_loc = op2_loc;

    InstructionNode* assign2 = parse_assign_stmt(); 
    lexer.GetToken();

    InstructionNode* body = parse_body();

    InstructionNode* temp = body;
    while (temp->next != nullptr)
        temp = temp->next;
    temp->next = assign2;

    InstructionNode* jmp = new InstructionNode;
    jmp->type = JMP;
    jmp->jmp_inst.target = cjmp;
    jmp->next = nullptr;

    assign2->next = jmp;

    InstructionNode* noop = new InstructionNode;
    noop->type = NOOP;
    noop->next = nullptr;

    jmp->next = noop;
    cjmp->cjmp_inst.target = noop;

    assign1->next = cjmp;
    cjmp->next = body;

    return assign1;
}

InstructionNode* parse_switch_stmt() {
    lexer.GetToken(); 
    Token var_token = lexer.GetToken();
    int var_loc;

    if (var_token.token_type == ID) {
        var_loc = get_or_add_var_location(var_token.lexeme);
    } else if (var_token.token_type == NUM) {
        var_loc = store_constant(stoi(var_token.lexeme));
    } else {
        cout << "Syntax error: SWITCH must be followed by ID or NUM\n";
        exit(1);
    }

    lexer.GetToken(); 

    InstructionNode* head = nullptr;
    InstructionNode* tail = nullptr;

    InstructionNode* end = new InstructionNode;
    end->type = NOOP;
    end->next = nullptr;

    std::set<int> seen_cases;
    Token t = lexer.peek(1);
    while (t.token_type == CASE) {
        lexer.GetToken(); 
        Token num_token = lexer.GetToken();
        int case_val = stoi(num_token.lexeme);
        lexer.GetToken(); 

        if (seen_cases.count(case_val)) {
            cout << "Warning: Duplicate CASE value " << case_val << " ignored\n";
            parse_body(); 
            t = lexer.peek(1);
            continue;
        }
        seen_cases.insert(case_val);

        int const_loc = store_constant(case_val);

        InstructionNode* cjmp = new InstructionNode;
        cjmp->type = CJMP;
        cjmp->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
        cjmp->cjmp_inst.op1_loc = var_loc;
        cjmp->cjmp_inst.op2_loc = const_loc;
        cjmp->next = nullptr;

        InstructionNode* body = parse_body();

        InstructionNode* jmp = new InstructionNode;
        jmp->type = JMP;
        jmp->jmp_inst.target = end;
        jmp->next = nullptr;

        InstructionNode* temp = body;
        while (temp->next != nullptr) temp = temp->next;
        temp->next = jmp;

        cjmp->cjmp_inst.target = body;

        if (head == nullptr) {
            head = cjmp;
            tail = cjmp;
        } else {
            tail->next = cjmp;
            tail = cjmp;
        }

        t = lexer.peek(1);
    }

    if (t.token_type == DEFAULT) {
        lexer.GetToken();
        lexer.GetToken(); 

        InstructionNode* def_body = parse_body();

        InstructionNode* temp = def_body;
        while (temp->next != nullptr) temp = temp->next;

        InstructionNode* jmp = new InstructionNode;
        jmp->type = JMP;
        jmp->jmp_inst.target = end;
        jmp->next = nullptr;

        temp->next = jmp;

        if (tail)
            tail->next = def_body;
        else
            head = def_body;
    } else {
        if (tail)
            tail->next = end;
        else
            head = end;
    }

    lexer.GetToken(); 

    return head;
}

void parse_inputs() {
    Token t = lexer.GetToken();
    while (t.token_type == NUM) {
        inputs.push_back(stoi(t.lexeme));
        t = lexer.GetToken();
    }
}

//new switch
struct InstructionNode * parseSwitchStmt() {
    struct InstructionNode * i = new InstructionNode();
    struct InstructionNode * nooper = new InstructionNode();
    nooper->type = NOOP;
    nooper->next = nullptr;

    expect(SWITCH);
    Token s = expect(ID);
    expect(LBRACE);
    i = parseCaseList(s,nooper);

    Token t = lexer.peek(1);
    if(t.token_type == RBRACE){
        //no default, connect last case->next to nooper
        struct InstructionNode * temp = i;
        while(temp->next != nullptr){
            temp = temp->next;
        }
        temp->next = nooper;
        //connect last case target (body) end node to nooper
        struct InstructionNode * temp2 = temp->cjmp_inst.target;
        while(temp2->next != nullptr){ 
            temp2 = temp2->next;
        }
        temp2->next = nooper;

        expect(RBRACE);
        return i;
    }
    else if(t.token_type == DEFAULT){
        //grab default and connect end of default body to nooper
        struct InstructionNode * defalt = parseDefaultCase();
        struct InstructionNode * eOfDefalt = defalt;
        while(eOfDefalt->next != nullptr){
            eOfDefalt = eOfDefalt->next;
        }
        eOfDefalt->next = nooper;

        //connect last case->next to default and last case target (body) end node to default
        struct InstructionNode * temp = i;
        while(temp->next != nullptr){
            temp = temp->next;
        }
        temp->next = defalt;
        //connect last case target (body) end node to nooper
        struct InstructionNode * temp2 = temp->cjmp_inst.target;
        while(temp2->next != nullptr){
            temp2 = temp2->next;
        }
        temp2->next = defalt;

        expect(RBRACE);
        return i;
    }
    else
        syntax_error();
    return new InstructionNode();
}

//new caselist
struct InstructionNode * parseCaseList(Token s, struct InstructionNode * noop) {
    struct InstructionNode * i = new InstructionNode();
    
    i = parseCase(s,noop);
    Token t = lexer.peek(1);

    //if another case, set next (condition NOT_EQUAL is true) to the next case
    //and connect jumper->next at the end of i's case to the next case
    if(t.token_type == CASE){
        struct InstructionNode * nextCase = parseCaseList(s,noop);
        //connect case->next to next case
        i->next = nextCase;

        //connect case->target (body) end to nextCase
        struct InstructionNode * temp = i->cjmp_inst.target;
        while(temp->next != nullptr){
            temp = temp->next;
        }
        temp->next = nextCase;
        return i;
    }
    else if(t.token_type == DEFAULT || t.token_type == RBRACE){
        i->next = nullptr;
        return i;
    }
    else
        syntax_error();
    return new InstructionNode();
}

//new case
struct InstructionNode * parseCase(Token s, struct InstructionNode* noop) {
    struct InstructionNode * i = new InstructionNode();
    struct InstructionNode * jumper = new InstructionNode();
    i->type = CJMP;
    i->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
    jumper->type = JMP;
    jumper->jmp_inst.target = noop;
    jumper->next = nullptr;
    
    expect(CASE);
    Token t = expect(NUM);
    //allocated memory for num

    bool stored = false;
    if(variable_table.find(t.lexeme) == variable_table.end()){
        variable_table[t.lexeme] = mem_index;
        stored = true;
    }

    if(stored){
        mem[mem_index] = stoi(t.lexeme);
        mem_index++;
    }

    int var_loc = variable_table.find(s.lexeme) == variable_table.end() ? 0 : variable_table[s.lexeme];
    int const_loc = variable_table.find(t.lexeme) == variable_table.end() ? 0 : variable_table[t.lexeme];

    i->cjmp_inst.op1_loc = var_loc;
    i->cjmp_inst.op2_loc = const_loc;

    expect(COLON);
    struct InstructionNode * body = parse_body();
    i->cjmp_inst.target = body;

    struct InstructionNode * temp = body;
    while(temp->next != nullptr){
        temp = temp->next;
    }
    temp->next = jumper;
    return i;
}

//new default
struct InstructionNode * parseDefaultCase() {
    expect(DEFAULT);
    expect(COLON);
    struct InstructionNode * i = new InstructionNode();
    i = parse_body();
    return i;
}