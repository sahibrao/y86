#include <stdio.h>
#include <stdlib.h>
#include "tester.h"


/* 
 * is_equal compares two y86 machine states for equivalence.
 * It returns 1 if s1 and s2 are equivalent.
 *
 * Unusual conditions: 
 * The memory states only need to match on valid bytes in memory.
 * The register states only need to match on valid y86 registers.
 * The flag bits only need to match on the specific flags supported
 * by the y86.
 */
int is_equal(y86_state_t *s1, y86_state_t *s2){
	// TODO: Check Memory in Valid Area (0 <-> start_addr + valid_mem)

	if((s1->valid_mem != s2->valid_mem) || (s1->start_addr != s2->start_addr)){
		printf("valid memory or start addr is wrong\n");
		return 0;
	}

	// int max = (s1->start_addr+s1->valid_mem > 1024)? 1024: (s1->start_addr+s1->valid_mem);
	int invalid = 0;
	for(int i = 0; i < s1->valid_mem; i++){ // checking each byte of memory // max addr? 
		if(s1->memory[i]!=s2->memory[i]){
			// printf("for %d, %d and %d\n", i, s1->memory[i], s2->memory[i]);
			printf("memory at %X is different\n", i);

			return 0;
			invalid = 1;
		}
	}
	if(invalid){
		return 0;
	}

	// TODO: Check Registers (checking 0-14?)
	for(int i = 0; i < 15; i++){
		if(s1->registers[i]!=s2->registers[i]){
			printf("register at %d is different", i);
			printf("\nmine is %1lX and answe is %1lX\n", s1->registers[i], s2->registers[i]);
			return 0;
		}
	}

	// Check PC
	if(s1->pc != s2->pc){
		printf("pc is different\n");
		return 0;
	}

	if((((s1->flags >> 6) & 0x01))^(((s2->flags >> 6) & 0x01))){
		printf("z flag is different\n");
		return 0;
	}
	if((((s1->flags >> 2) & 0x01))^(((s2->flags >> 2) & 0x01))){
		printf("s flag is different\n");
		return 0;
	}

	// s1==s2
	return 1;
}

/*
 * read_quad reads the 8-byte value at 'address' from the memory of
 * the machine state 'state' and stores the result in 'value.'
 * It returns 1 if a read is successful and 0 if it fails.
 */
int read_quad(y86_state_t *state, uint64_t address, uint64_t *value) {

	// Boundaries
	if(address + 8 > (state->start_addr+state->valid_mem)){
		return 0;
	}
	if(address < state->start_addr){ 
		return 0;
	}

	memcpy(value, state->memory + (address - state->start_addr), sizeof(uint64_t));

	return 1;
}

/* 
 * write_quad writes the 8-byte item 'value' to the machine state at memory
 * address 'address'.
 * It returns 1 if a write is successful and 0 if it fails.
 */
int write_quad(y86_state_t *state, uint64_t address, uint64_t value) {

	if((address + 8) > (state->start_addr + state->valid_mem)){
		return 0;
	}
	if(address < state->start_addr){ 
		return 0;
	}

	// memcpy(state->memory + (address - state->start_addr), &value, sizeof(uint64_t));

	*(uint64_t *)(state->memory + (address - state->start_addr)) = value;

	return 1;
}

void create_copy_state(y86_state_t *s1, y86_state_t *s2){
	s2->flags = s1->flags;
	s2->pc = s1->pc;
	s2->start_addr = s1->start_addr;
	s2->valid_mem = s1->valid_mem;
	for (int i = 0; i < 16; i++)
	{
		s2->registers[i] = s1->registers[i];
	}
	for (int i = 0; i < 1024; i++)
	{
		s2->memory[i] = s1->memory[i];
	}
}

void condcode(y86_state_t *state, uint64_t value){
	if(value==0){ // bit 7 needs to be 1
		if(((state->flags >> 6) & 0x01) == 1){ 
				// already one
			} else {
				state->flags = state->flags + 0x40;
				// added a that bit
			}
	} else { // bit 7 needs to be 0
		if(((state->flags >> 6) & 0x01) == 1){ //
				state->flags = state->flags - 0x40;
				// took away that bit
			}
	}
	if(value>>63){
		if(((state->flags >> 2) & 0x01) == 1){
			// alreadu one
		} else {
			state->flags = state->flags + 0x04;
		}
	} else {
		if(((state->flags >> 2) & 0x01) == 1){
			state->flags = state->flags - 0x04;
		}
	}
}

int condtionCheckEQ(uint8_t flags){
	return (flags>>6) & 0x01;
}

int condtionCheckL(uint8_t flags){
	return (flags>>2) & 0x01;
}

int api(y86_state_t *state, y86_inst_t inst){

	// 1. use inst_to_enum function to turn into enum
	// 2. check if instruction is valid
	// 3. if register value needed, check if reg numbers are valid
	// 4. if need to access memory, compute address
	// 5. if computed address, check if valid
	// 6. if nec, read/write memory
	// 7. if nec, write values into reg
	// 8. Update PC

	inst_t instruction = inst_to_enum(inst.instruction);

	// 2 how to check if the instruction is valid

	if(instruction == I_NOP){ 
		// printf("run nop instruction\n"); -- DONE
		state->pc = state->pc+1;
		return 1;

	} else if(instruction == I_HALT || instruction == I_INVALID){ 
		// printf("halt instruction\n");
		return 0;

	} else if(instruction == 2){
		printf("rrmovq instruction\n");
		uint64_t valP = state->pc+2;

		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		
		uint64_t valA = state->registers[inst.rA];

		uint64_t valE = valA + 0;

		state->registers[inst.rB] = valE;

		state->pc = valP;

		return 1;

	} else if(instruction == 3){
		// printf("irmovq instruction\n");
		uint64_t valP = state->pc+10;

		if(inst.rB > 0xe){
			printf("invalid register");
			return 0;
		}

		uint64_t valC = inst.constval;

		uint64_t valE = valC + 0;

		// need to set R[RB] <- R[RA]
		state->registers[inst.rB] = valE;

		state->pc = valP;

		return 1;

	} else if(instruction == 4){
		printf("rmmovq instruction\n");
		uint64_t valP = state->pc+10;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];
		uint64_t valB = state->registers[inst.rB];
		uint64_t valC = inst.constval;

		uint64_t valE = valC + valB;


		// need to M8[valE] ← valA
		int x = write_quad(state, valE, valA);
		if (!x) {
			return 0;
		}
		state->pc = valP;

		return 1;

	} else if(instruction == 5){
		printf("mrmovq instruction\n");
		uint64_t valP = state->pc+10;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valB = state->registers[inst.rB];
		uint64_t valC = inst.constval;

		uint64_t valE = valC + valB;
		if(valE >= (state->start_addr + state->valid_mem)){
			printf("invalid address");
			return 0;
		}

		// need to valM ← M8[valE]
		uint64_t valM;
		int x = read_quad(state, valE, &valM);
		if (!x) { // I think this fixed it?
			return 0;
		}

		state->registers[inst.rA] = valM;

		state->pc = valP;

		return 1;
	} else if(instruction == I_ADDQ){
		printf("addq instruction\n");
		uint64_t valP = state->pc+2;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];
		uint64_t valB = state->registers[inst.rB];

		uint64_t valE = valA + valB;

		condcode(state, valE);

		state->registers[inst.rB] = valE;

		state->pc = valP;

		return 1;
	} else if(instruction == I_SUBQ){
		printf("subq instruction\n");
		uint64_t valP = state->pc+2;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];
		uint64_t valB = state->registers[inst.rB];

		uint64_t valE = valB - valA;

		condcode(state, valE);

		state->registers[inst.rB] = valE;

		state->pc = valP;

		return 1;
	} else if(instruction == I_MULQ){
		printf("mulq instruction\n");
		uint64_t valP = state->pc+2;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];
		uint64_t valB = state->registers[inst.rB];

		uint64_t valE = valB * valA;

		condcode(state, valE);

		state->registers[inst.rB] = valE;

		state->pc = valP;

		return 1;
	} else if(instruction == I_MODQ){
		printf("modq instruction\n");
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valP = state->pc+2;
		int64_t valA = state->registers[inst.rA];
		int64_t valB = state->registers[inst.rB];

		if(valA==0){
			return 0;
		}

		uint64_t valE = (uint64_t)(valB%valA); // check first bit

		condcode(state, valE);

		state->registers[inst.rB] = valE;

		state->pc = valP;

		return 1;
	} else if(instruction == I_DIVQ){
		printf("divq instruction\n");
		if((inst.rA > 0xe) || (inst.rB > 0xe)){ // check 0 
			printf("invalid register");
			return 0;
		}
		uint64_t valP = state->pc+2;
		int64_t valA = state->registers[inst.rA];
		int64_t valB = state->registers[inst.rB];
		uint64_t valE;
		
		if(valA==0){
			return 0;
		} else {
			valE = (uint64_t)(valB/valA);
		}

		condcode(state, valE);

		state->registers[inst.rB] = valE;

		state->pc = valP;

		return 1;
	} else if(instruction == I_ANDQ){
		printf("andq instruction\n");
		uint64_t valP = state->pc+2;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];
		uint64_t valB = state->registers[inst.rB];

		uint64_t valE = valB&valA;

		condcode(state, valE);

		state->registers[inst.rB] = valE;

		state->pc = valP;

		return 1;
	} else if(instruction == I_XORQ){
		printf("xor instruction\n");
		uint64_t valP = state->pc+2;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];
		uint64_t valB = state->registers[inst.rB];

		uint64_t valE = valB^valA;
		printf("valE is %1lX", valE);

		condcode(state, valE);

		state->registers[inst.rB] = valE;

		state->pc = valP;

		return 1;
	} else if(instruction == I_CMOVEQ){
		printf("cmoveq instruction\n");
		uint64_t valP = state->pc+2;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];

		uint64_t valE = 0+valA;

		// if condition codes
		if(condtionCheckEQ(state->flags)){
			state->registers[inst.rB] = valE;
		}

		state->pc = valP;

		return 1;
	} else if(instruction == I_CMOVNE){
		printf("cmoveq instruction\n");
		uint64_t valP = state->pc+2;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];

		uint64_t valE = 0+valA;

		// if condition codes
		if(!condtionCheckEQ(state->flags)){
			state->registers[inst.rB] = valE;
		}

		state->pc = valP;

		return 1;
	} else if(instruction == I_CMOVL){
		printf("cmovl instruction\n");
		uint64_t valP = state->pc+2;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];

		uint64_t valE = 0+valA;

		// if condition codes
		if(condtionCheckL(state->flags)){
			state->registers[inst.rB] = valE;
		}

		printf("state reg rB is %1lX\n", state->registers[inst.rB]);

		state->pc = valP;

		return 1;
	} else if(instruction == I_CMOVLE){
		printf("cmovle instruction\n");
		uint64_t valP = state->pc+2;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];

		uint64_t valE = 0+valA;

		// if condition codes
		if(condtionCheckL(state->flags)||condtionCheckEQ(state->flags)){
			state->registers[inst.rB] = valE;
		}

		state->pc = valP;

		return 1;
	} else if(instruction == I_CMOVG){
		printf("cmovle instruction\n");
		uint64_t valP = state->pc+2;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];

		uint64_t valE = 0+valA;

		// if condition codes
		if(!(condtionCheckL(state->flags)||condtionCheckEQ(state->flags))){
			state->registers[inst.rB] = valE;
		}

		state->pc = valP;

		return 1;
	} else if(instruction == I_CMOVGE){
		printf("cmovle instruction\n");
		uint64_t valP = state->pc+2;
		if((inst.rA > 0xe) || (inst.rB > 0xe)){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];

		uint64_t valE = 0+valA;

		// if condition codes
		if(!(condtionCheckL(state->flags))){
			state->registers[inst.rB] = valE;
		}

		state->pc = valP;

		return 1;
	} else if(instruction == I_PUSHQ){
		printf("pushq instruction\n");

		
		
		uint64_t valP = state->pc+2;
		if(inst.rA > 0xe){
			printf("invalid register");
			return 0;
		}
		uint64_t valA = state->registers[inst.rA];
		uint64_t valB = state->registers[4]; // %rsp

		uint64_t valE = valB - 8;

		if(valE >= (state->start_addr + state->valid_mem)){
			printf("invalid address");
			return 0;
		}
	
		int x = write_quad(state, valE, valA);
		if(!x){
			return 0;
		}

		state->registers[4] = valE;

		state->pc = valP;

		return 1;
	} else if(instruction == I_POPQ){
		printf("popq instruction\n");
		uint64_t valP = state->pc+2;
		uint64_t valA = state->registers[4]; // %rsp
		uint64_t valB = state->registers[4]; // %rsp

		uint64_t valE = valB + 8;
		if(inst.rA > 0xe){
			printf("invalid register");
			return 0;
		}

		if(valA >= (state->start_addr + state->valid_mem)){
			printf("invalid address");
			return 0;
		}

		uint64_t valM;
		int x = read_quad(state, valA, &valM);
		if(!x){
			return 0;
		}

		if(valE >= (state->start_addr + state->valid_mem)){
			printf("invalid address");
			return 0;
		}

		state->registers[4] = valE;
		state->registers[inst.rA] = valM;

		state->pc = valP;

		return 1;
	} else if(instruction == I_J){
		printf("jmp instruction\n");
		uint64_t valC = inst.constval;

		state->pc = valC;

		return 1;
	} else if(instruction == I_JEQ){
		printf("jeq instruction\n");
		uint64_t valC = inst.constval;

		if(condtionCheckEQ(state->flags)){
			state->pc = valC;
		} else {
			state->pc = state->pc+9;
		}

		return 1;
	} else if(instruction == I_JNE){
		printf("jne instruction\n");
		uint64_t valC = inst.constval;

		if(!condtionCheckEQ(state->flags)){
			state->pc = valC;
		} else {
			state->pc = state->pc+9;
		}

		return 1;
	} else if(instruction == I_JL){
		printf("jl instruction\n");
		uint64_t valC = inst.constval;

		if(condtionCheckL(state->flags)){
			state->pc = valC;
		} else {
			state->pc = state->pc+9;
		}

		return 1;
	} else if(instruction == I_JLE){
		printf("jle instruction\n");
		// uint64_t valP = state->pc+9;
		uint64_t valC = inst.constval;

		if(condtionCheckL(state->flags)||condtionCheckEQ(state->flags)){
			state->pc = valC;
		} else {
			state->pc = state->pc+9;
		}

		return 1;
	} else if(instruction == I_JG){
		printf("jle instruction\n");
		// uint64_t valP = state->pc+9;
		uint64_t valC = inst.constval;

		if(!(condtionCheckL(state->flags)||condtionCheckEQ(state->flags))){
			state->pc = valC;
		} else {
			state->pc = state->pc+9;
		}

		return 1;
	} else if(instruction == I_JGE){
		printf("jle instruction\n");
		uint64_t valC = inst.constval;

	
		if(!(condtionCheckL(state->flags))){
			state->pc = valC;
		} else {
			state->pc = state->pc+9;
		}
		return 1;

	} else if (instruction == I_RET) {
		printf("ret istruction\n");
		uint64_t valM;
		uint64_t stackPtr = state->registers[4]; 
		int x = read_quad(state, stackPtr, &valM);
		if (!x) {
			return 0;
		}
		state->registers[4] += 8;
		state->pc = valM;
		return 1;

	} else if (instruction == I_CALL) {
		printf("call istruction\n");
		uint64_t valC = inst.constval;
		if (valC >= state->start_addr + state->valid_mem) {
			return 0;
		}
		state->registers[4] -= 8;
		
		int x = write_quad(state, state->registers[4], state->pc + 9);
		if (!x) {
			return 0;
		}
		state->pc = valC;
		return 1;
	} else {
		return 0;
	}
}

y86_state_t create_empty_st()
{
	y86_state_t state;
	state.flags = 0;
	state.pc = 0;
	state.start_addr = 0;
	state.valid_mem = 0;
	for (int i = 0; i < 16; i++)
	{
		state.registers[i] = 0;
	}
	for (int i = 0; i < 1024; i++)
	{
		state.memory[i] = 0;
	}
	return state;
}

/*
 * y86_check returns 0 if the y86sim_func properly simulates
 * the n_inst instructions described in the instructions array, and
 * non-zero otherwise. Note that even though the simulator should update
 * the value of the program counter as a real program would, it always
 * executes the n_inst instructions sequentially. That is, instructions
 * such as call, ret, or jXX do not change which instruction is to be
 * executed next.
 *
 * Hint: To validate that 'simfunc' executed properly, you will need
 * to produce the correct end state. That means that you must write
 * your own simulator to produce that correct end state.
 *
 * Unusual conditions:
 * On halt: stop execution, the state should reflect the last executed
 * 	instruction.
 * On an invalid command: stop execution, the state should reflect the last
 * 	executed instruction.
 * On any bad arguments, e.g., invalid register number or access to an
 *	invalid address, divide by 0, stop execution, the state should
 * 	reflect the last executed instruction
 */
int y86_check(y86_state_t *state, y86_inst_t *instructions, int n_inst, y86sim_func simfunc) {



	// 1 (Might need to optimize this later, very slow)
	y86_state_t copystate = *state;

	// 2 Call all the instructions onto the state
	for(int i = 0; i < n_inst; i++){
		int x = api(state, instructions[i]);
		if(!x){
			break;
		}
	}

	// simfunc(&copystate, instructions, n_inst);	

	return !is_equal(state, &copystate);

}
