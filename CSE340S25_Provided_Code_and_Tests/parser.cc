#include <iostream>
#include <map>
#include <vector>
#include <string>
#include "lexer.h"
#include "execute.h"

using namespace std;

LexicalAnalyzer lexer;
map<string, int> variable_table;
int mem_index = 0;

// Maps variable names or stores constants into mem[]
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

InstructionNode* parse_stmt();
InstructionNode* parse_if_stmt();
InstructionNode* parse_assign_stmt();
InstructionNode* parse_input_stmt();
InstructionNode* parse_output_stmt();
InstructionNode* parse_stmt_list();
InstructionNode* parse_body();
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

void parse_inputs() {
    Token t = lexer.GetToken();
    while (t.token_type == NUM) {
        inputs.push_back(stoi(t.lexeme));
        t = lexer.GetToken();
    }
}
