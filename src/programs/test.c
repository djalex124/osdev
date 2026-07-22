int main(void)
{
    asm("mov $3, %rax; int $0x40");

    return 42;
}