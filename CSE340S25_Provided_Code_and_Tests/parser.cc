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


InstructionNode* parseSwitchStmt();

InstructionNode* parse_switch_case_list(Token, InstructionNode*);
InstructionNode* parse_single_case(Token, InstructionNode*);
InstructionNode* parse_default_case();


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

void parse_inputs() {
    Token t = lexer.GetToken();
    while (t.token_type == NUM) {
        inputs.push_back(stoi(t.lexeme));
        t = lexer.GetToken();
    }
}

InstructionNode* parse_switch_stmt() {
    InstructionNode* result = new InstructionNode();
    InstructionNode* exit_node = new InstructionNode();
    exit_node->type = NOOP;
    exit_node->next = nullptr;

    expect(SWITCH);
    Token switch_var = expect(ID);
    expect(LBRACE);
    result = parse_switch_case_list(switch_var, exit_node);

    Token lookahead = lexer.peek(1);
    if (lookahead.token_type == RBRACE) {
        InstructionNode* last = result;
        while (last->next != nullptr) last = last->next;
        last->next = exit_node;

        InstructionNode* body_end = last->cjmp_inst.target;
        while (body_end->next != nullptr) body_end = body_end->next;
        body_end->next = exit_node;

        expect(RBRACE);
        return result;
    }
    else if (lookahead.token_type == DEFAULT) {
        InstructionNode* default_block = parse_default_case();
        InstructionNode* default_tail = default_block;
        while (default_tail->next != nullptr) default_tail = default_tail->next;
        default_tail->next = exit_node;

        InstructionNode* last = result;
        while (last->next != nullptr) last = last->next;
        last->next = default_block;

        InstructionNode* body_end = last->cjmp_inst.target;
        while (body_end->next != nullptr) body_end = body_end->next;
        body_end->next = default_block;

        expect(RBRACE);
        return result;
    } else {
        syntax_error();
        return new InstructionNode();
    }
}

InstructionNode* parse_switch_case_list(Token switch_token, InstructionNode* end_jump) {
    InstructionNode* first_case = parse_single_case(switch_token, end_jump);
    Token next = lexer.peek(1);

    if (next.token_type == CASE) {
        InstructionNode* more_cases = parse_switch_case_list(switch_token, end_jump);
        first_case->next = more_cases;

        InstructionNode* case_tail = first_case->cjmp_inst.target;
        while (case_tail->next != nullptr) case_tail = case_tail->next;
        case_tail->next = more_cases;
        return first_case;
    }
    else if (next.token_type == DEFAULT || next.token_type == RBRACE) {
        first_case->next = nullptr;
        return first_case;
    }
    else {
        syntax_error();
        return new InstructionNode();
    }
}

InstructionNode* parse_single_case(Token switch_token, InstructionNode* end_node) {
    expect(CASE);
    Token num_token = expect(NUM);
    expect(COLON);

    InstructionNode* condition = new InstructionNode();
    InstructionNode* exit_jump = new InstructionNode();
    condition->type = CJMP;
    condition->cjmp_inst.condition_op = CONDITION_NOTEQUAL;

    exit_jump->type = JMP;
    exit_jump->jmp_inst.target = end_node;
    exit_jump->next = nullptr;

    bool new_const = false;
    if (variable_table.find(num_token.lexeme) == variable_table.end()) {
        variable_table[num_token.lexeme] = mem_index;
        new_const = true;
    }
    if (new_const) {
        mem[mem_index] = stoi(num_token.lexeme);
        mem_index++;
    }

    int lhs = get_or_add_var_location(switch_token.lexeme);
    int rhs = get_or_add_var_location(num_token.lexeme);

    condition->cjmp_inst.op1_loc = lhs;
    condition->cjmp_inst.op2_loc = rhs;

    InstructionNode* body = parse_body();
    InstructionNode* body_tail = body;
    while (body_tail->next != nullptr) body_tail = body_tail->next;
    body_tail->next = exit_jump;

    condition->cjmp_inst.target = body;
    condition->next = body;
    return condition;
}

InstructionNode* parse_default_case() {
    expect(DEFAULT);
    expect(COLON);
    return parse_body();
}
