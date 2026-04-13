#include <iostream> 
#include <cstdlib>

using namespace std;

int main(){
    srand(time(0));

//generate random number 1 - 100

int randomNum,attempt,attemptUser;
attemptUser=0;
randomNum = rand() % 100+1;


// make an attempt & save it
do {
        cout<<" Number= ";
        cin>>attempt;
        attemptUser++;
        

        if(attempt<randomNum) {

            cout<<"Too low"<<endl;
        }
else if(attempt>randomNum) {
    cout<<"Too high "<<endl;

} else {
    cout<< "Congrats"<<endl;
    cout<<attemptUser<<" tries"<<endl;
    break;
}


} while(attempt != randomNum);

return 0;

}