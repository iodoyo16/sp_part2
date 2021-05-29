#include<stdio.h>
#include<unistd.h>
int main(){
    int n;
    scanf("%d",&n);
    for(int i=0;i<n;i++){
        sleep(2);
        printf("%d\n",i);
    }

}