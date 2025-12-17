void print(char *str) {
	int i = 0;
	while(str[i]) {
		asm volatile("int $0x80" :: "a"(1), "b"(str[i]));
		i++;
	}
}

int main() {
    int x = 0;
    x = x + 1;

	unsigned long long int y = 0;
	y = y + 1;

    print("halo andra\n");
    
    return 0;
}
