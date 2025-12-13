int main() {
    int x = 0;
    x = x + 1;
    
    // Simple asm without constraints - just the instruction
    asm volatile("mov $1, %eax");
    asm volatile("mov $97, %ebx");  // 'a'
    asm volatile("int $0x80");
    
    asm volatile("mov $1, %eax");
    asm volatile("mov $110, %ebx");  // 'n'
    asm volatile("int $0x80");
    
    asm volatile("mov $1, %eax");
    asm volatile("mov $100, %ebx");  // 'd'
    asm volatile("int $0x80");
    
    asm volatile("mov $1, %eax");
    asm volatile("mov $114, %ebx");  // 'r'
    asm volatile("int $0x80");
    
    asm volatile("mov $1, %eax");
    asm volatile("mov $97, %ebx");   // 'a'
    asm volatile("int $0x80");
    
    return 0;
}