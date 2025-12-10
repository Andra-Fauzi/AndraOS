#include <stdint.h>

void kprint(char *str) {
	int i = 0;
	while(str[i]) {
		asm volatile("int $0x80" :: "a"(1), "b"(str[i]));
		i++;
	}
}

void print_char(char c) {
	asm volatile("int $0x80" :: "a"(1), "b"(c));
}

char getinput() {
	uint32_t c = (uint32_t)0;
	asm volatile("int $0x80" :: "a"(2), "b"(c));
	asm volatile("mov %0, %%ebx" : "=b"(c));
	return (char)c;
}

int main() {
	kprint("Hello World\n");
	kprint("Zilfa\n");
	char buffer[255];
	int i = 0;
	char input;
	while(true) {
		input = getinput();
		print_char(input);
		if(input == '\n') {
			break;
		}
	}
	return 0;
}
