#include "Chip8.hpp"

const unsigned int START_ADDRESS = 0x200;
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;

uint8_t fontset[FONTSET_SIZE] = 
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void Chip8::LoadROM(char const* filename) {

	// Open file as binary stream & put file pointer @ EOF
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	if (file.is_open()) {
		// Get size of file and allocate buffer to hold it
		std::streampos size = file.tellg();
		char* buffer = new char[size]; 

		// Go back to beginning of file & read into buffer
		file.seekg(0, std::ios::beg);
		file.read(buffer, size);
		file.close();

		// Load the ROM contents into the CHIP-8's memory, starting at 0x200
		for (long i = 0; i < size; i++) {
			memory[START_ADDRESS + i] = buffer[i];
		}

		// Free buffer, since it's no longer needed
		delete[] buffer;
	}
}

Chip8::Chip8()
	: randGen(std::chrono::system_clock::now().time_since_epoch().count())
{

	// Initialize program counter
	pc = START_ADDRESS;

	// Load fonts into memory
	for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
		memory[FONTSET_START_ADDRESS + i] = fontset[i];
	}

	// Initialize RNG
	randByte = std::uniform_int_distribution<short>(0, 255U); // NOTE: when doing things with this, cast short to uint8_t

	// Set up function pointer table
	table[0x0] = &Chip8::Table0;
	table[0x1] = &Chip8::OP_1nnn;
	table[0x2] = &Chip8::OP_2nnn;
	table[0x3] = &Chip8::OP_3xkk;
	table[0x4] = &Chip8::OP_4xkk;
	table[0x5] = &Chip8::OP_5xy0;
	table[0x6] = &Chip8::OP_6xkk;
	table[0x7] = &Chip8::OP_7xkk;
	table[0x8] = &Chip8::Table8;
	table[0x9] = &Chip8::OP_9xy0;
	table[0xA] = &Chip8::OP_Annn;
	table[0xB] = &Chip8::OP_Bnnn;
	table[0xC] = &Chip8::OP_Cxkk;
	table[0xD] = &Chip8::OP_Dxyn;
	table[0xE] = &Chip8::TableE;
	table[0xF] = &Chip8::TableF;

	table0[0x0] = &Chip8::OP_00E0;
	table0[0xE] = &Chip8::OP_00EE;

	table8[0x0] = &Chip8::OP_8xy0;
	table8[0x1] = &Chip8::OP_8xy1;
	table8[0x2] = &Chip8::OP_8xy2;
	table8[0x3] = &Chip8::OP_8xy3;
	table8[0x4] = &Chip8::OP_8xy4;
	table8[0x5] = &Chip8::OP_8xy5;
	table8[0x6] = &Chip8::OP_8xy6;
	table8[0x7] = &Chip8::OP_8xy7;
	table8[0xE] = &Chip8::OP_8xyE;

	tableE[0x1] = &Chip8::OP_ExA1;
	tableE[0xE] = &Chip8::OP_Ex9E;

	tableF[0x07] = &Chip8::OP_Fx07;
	tableF[0x0A] = &Chip8::OP_Fx0A;
	tableF[0x15] = &Chip8::OP_Fx15;
	tableF[0x18] = &Chip8::OP_Fx18;
	tableF[0x1E] = &Chip8::OP_Fx1E;
	tableF[0x29] = &Chip8::OP_Fx29;
	tableF[0x33] = &Chip8::OP_Fx33;
	tableF[0x55] = &Chip8::OP_Fx55;
	tableF[0x65] = &Chip8::OP_Fx65;
}

void Chip8::Cycle() {
	// Fetch
	opcode = (memory[pc] << 8u) | memory[pc + 1];

	// Increment PC before executing instruction
	pc += 2;

	// Decode & Execute
	((*this).*(table[(opcode & 0xF000u) >> 12u]))();

	// Decrement the delay timer if it's been set
	if (delayTimer > 0) {
		--delayTimer;
	}

	// Decrement sound timer if it's been set
	if (soundTimer > 0) {
		--soundTimer;
	}
}

// --------------------- INSTRUCTIONS --------------------------

void Chip8::Table0()
{
	((*this).*(table0[opcode & 0x000Fu]))();
}

void Chip8::Table8()
{
	((*this).*(table8[opcode & 0x000Fu]))();
}

void Chip8::TableE()
{
	((*this).*(tableE[opcode & 0x000Fu]))();
}

void Chip8::TableF()
{
	((*this).*(tableF[opcode & 0x00FFu]))();
}

void Chip8::OP_NULL()
{}

// CLS - clear screen
void Chip8::OP_00E0() {
	memset(video, 0, sizeof(video));
}

// RET - return
void Chip8::OP_00EE() {
	--sp;
	pc = stack[sp];
}

// 1nnn - JMP - jump to location nnn -> sets pc to nnn
void Chip8::OP_1nnn() {
	uint16_t address = opcode & 0x0FFFu;
	pc = address;
}

// 2nnn - CALL addr - call subroutine at nnn
void Chip8::OP_2nnn() {
	uint16_t address = opcode & 0x0FFFu;
	stack[sp] = pc;
	++sp;
	pc = address;
}

// 3xkk - SE Vx - Skip next instruction if Vx = kk (Skip Equal)
void Chip8::OP_3xkk() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;

	if (registers[Vx] == byte) {
		pc += 2;
	}
}

// 4xkk - SNE Vx - Skip next instruction if Vx != kk (Skip Not Equal)
void Chip8::OP_4xkk() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;
	if (registers[Vx] != byte) {
		pc += 2;
	}
}

// 5xy0 - SE Vx, Vy - Skip next instruction if Vx = Vy
void Chip8::OP_5xy0() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vx] == registers[Vy]) {
		pc += 2;
	}
}

// 6xkk - LD Vx, byte - Set Vx = kk
void Chip8::OP_6xkk() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;
	registers[Vx] = byte;
}

// 7xkk - ADD Vx, byte - Set Vx = Vx + kk
void Chip8::OP_7xkk() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;

	registers[Vx] += byte;
}

// 8xy0 - LD Vx, Vy - Set Vx = Vy
void Chip8::OP_8xy0() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	registers[Vx] = registers[Vy];
}

// 8xy1 - OR Vx, Vy - Set Vx = Vx OR Vy
void Chip8::OP_8xy1()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	registers[Vx] |= registers[Vy];
}

// 8xy2 - AND Vx, Vy - Set Vx = Vx AND Vy
void Chip8::OP_8xy2()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	registers[Vx] &= registers[Vy];
}

// 8xy3 - XOR Vx, Vy - Set Vx = Vx XOR Vy
void Chip8::OP_8xy3()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	registers[Vx] ^= registers[Vy];
}

// 8xy4 - ADD Vx, Vy 
// Set Vx = Vx + Vy, set VF = carry
// Vx + Vy are added, if result > 8 bits (i.e. 255), VF = 1 else VF = 0
void Chip8::OP_8xy4()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	uint16_t sum = registers[Vx] + registers[Vy];

	if (sum > 255U)	{
		registers[0xF] = 1;
	}
	else {
		registers[0xF] = 0;
	}

	registers[Vx] = sum & 0xFFu;
}

// 8xy5 - SUB Vx, Vy - Set Vx - Vx - Vy
// If Vx > Vy, VF = 1, else 0
// Then Vy is subtracted from Vx, & results stored in Vx
void Chip8::OP_8xy5()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vx] > registers[Vy])
	{
		registers[0xF] = 1;
	}
	else
	{
		registers[0xF] = 0;
	}

	registers[Vx] -= registers[Vy];
}

// 8xy6 - SHR Vx - If LSB of Vx is 1, then VF = 1 else 0.
// Then Vx is divided by 2
// Shift Right
void Chip8::OP_8xy6()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	// Save LSB in VF
	registers[0xF] = (registers[Vx] & 0x1u);

	registers[Vx] >>= 1;
}

// 8xy7 - SUBN Vx, Vy 
// Set Vx = Vy - Vx 
void Chip8::OP_8xy7()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vy] > registers[Vx])
	{
		registers[0xF] = 1;
	}
	else
	{
		registers[0xF] = 0;
	}

	registers[Vx] = registers[Vy] - registers[Vx];
}

// 8xyE - SHL Vx {, Vy}
// if MSB of Vx is 1, VF = 1, else 0
// Then Vx is multiplied by 2
// Left Shift
void Chip8::OP_8xyE()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	// Save MSB in VF
	registers[0xF] = (registers[Vx] & 0x80u) >> 7u;

	registers[Vx] <<= 1;
}

// 9xy0 - SNE Vx, Vy - Skip Not Equal
void Chip8::OP_9xy0()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (registers[Vx] != registers[Vy])
	{
		pc += 2;
	}
}

// Annn - LD I, addr
// Set I = nnn
void Chip8::OP_Annn()
{
	uint16_t address = opcode & 0x0FFFu;

	index = address;
}

// Bnnn - JP V0, addr
// Jump to (nnn + V0)
void Chip8::OP_Bnnn() {
	uint16_t address = opcode & 0x0FFFu;
	pc = registers[0] + address;
}

// Cxkk - RND Vx, byte
// Set Vx = random byte AND kkk
void Chip8::OP_Cxkk()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;

	registers[Vx] = (uint8_t)randByte(randGen) & byte;
}

// Dxyn - DRW Vx, Vy, nibble
// Display n-byte sprite starting @ mem loc I at (Vx, Vy) and set VF = collision
// Draw
void Chip8::OP_Dxyn() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	uint8_t height = opcode & 0x000Fu;

	// Wrap if going beyond screen boundaries
	uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
	uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

	registers[0xF] = 0;

	for (unsigned int row = 0; row < height; ++row) {
		uint8_t spriteByte = memory[index + row];

		for (unsigned int col = 0; col < 8; ++col) {
			uint8_t spritePixel = spriteByte & (0x80u >> col);
			uint32_t* screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

			// Sprite pixel is on
			if (spritePixel) {
				// Screen pixel also on - collision
				if (*screenPixel == 0xFFFFFFFF) {
					registers[0xF] = 1;
				}

				// Effectively XOR with the sprite pixel
				*screenPixel ^= 0xFFFFFFFF;
			}
		}
	}
}

// Ex9E - SKP VX - Skip next instruction if key w/ val of Vx is pressed
void Chip8::OP_Ex9E() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t key = registers[Vx];

	if (keypad[key]) {
		pc += 2;
	}
}

// ExA1 - SKNP Vx - Skip next instruction if key w/ val of Vx NOT pressed
void Chip8::OP_ExA1()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	uint8_t key = registers[Vx];

	if (!keypad[key])
	{
		pc += 2;
	}
}

// Fx07 - LD Vx, DT
// Set Vx = delay timer val
void Chip8::OP_Fx07()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	registers[Vx] = delayTimer;
}

// Fx0A - LD Vx, K
// Wait for a key press, store the value of the key in Vx
// Waits by looping the previous instruction / decrementing pc by 2 
// whenever key val NOT detected
void Chip8::OP_Fx0A()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	if (keypad[0])
	{
		registers[Vx] = 0;
	}
	else if (keypad[1])
	{
		registers[Vx] = 1;
	}
	else if (keypad[2])
	{
		registers[Vx] = 2;
	}
	else if (keypad[3])
	{
		registers[Vx] = 3;
	}
	else if (keypad[4])
	{
		registers[Vx] = 4;
	}
	else if (keypad[5])
	{
		registers[Vx] = 5;
	}
	else if (keypad[6])
	{
		registers[Vx] = 6;
	}
	else if (keypad[7])
	{
		registers[Vx] = 7;
	}
	else if (keypad[8])
	{
		registers[Vx] = 8;
	}
	else if (keypad[9])
	{
		registers[Vx] = 9;
	}
	else if (keypad[10])
	{
		registers[Vx] = 10;
	}
	else if (keypad[11])
	{
		registers[Vx] = 11;
	}
	else if (keypad[12])
	{
		registers[Vx] = 12;
	}
	else if (keypad[13])
	{
		registers[Vx] = 13;
	}
	else if (keypad[14])
	{
		registers[Vx] = 14;
	}
	else if (keypad[15])
	{
		registers[Vx] = 15;
	}
	else
	{
		pc -= 2;
	}
}

// Fx15 - LD DT, Vx
// Set delay timer = Vx
void Chip8::OP_Fx15()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	delayTimer = registers[Vx];
}

// Fx18 - LD ST. Vx
// Set sound timer = Vx
void Chip8::OP_Fx18()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	soundTimer = registers[Vx];
}

// Fx1E - ADD I, Vx
// Set I = I + Vx
void Chip8::OP_Fx1E()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	index += registers[Vx];
}

// Fx29 - LD F, Vx
// Set I = location fo sprite for digit Vx
void Chip8::OP_Fx29()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t digit = registers[Vx];

	index = FONTSET_START_ADDRESS + (5 * digit);
}

// Fx33 - LD B, Vx
// Store BCD representation of Vx in RAM locations I, I + 1, and I + 2
// Interpreter takes decimal val of Vx and places hundreds digit in RAM @ I,
// Tens digit in I + 1, and ones digit in I + 2
void Chip8::OP_Fx33()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t value = registers[Vx];

	// Ones-place
	memory[index + 2] = value % 10;
	value /= 10;

	// Tens-place
	memory[index + 1] = value % 10;
	value /= 10;

	// Hundreds-place
	memory[index] = value % 10;
}

// Fx55 - LD [I], Vx
// Store registers V0 through Vx in RAM starting at I
void Chip8::OP_Fx55() {
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; ++i) {
		memory[index + i] = registers[i];
	}
}

// Fx65 - LD Vx, [I]
// Read registers V0 through Vx from RAM starting at I
void Chip8::OP_Fx65()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; ++i)
	{
		registers[i] = memory[index + i];
	}
}

