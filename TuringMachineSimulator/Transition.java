/*
Nume: Catrina Mihaela-Florentina
Grupa: 323CB
E-mail: catrina.mihaela20@gmail.com
**/

class Transition {
    String nextState;
    char tape1WriteSymbol;
    private char tape1Direction;
    char tape2WriteSymbol;
    private char tape2Direction;

    Transition(String nextState, char tape1WriteSymbol, char tape1Direction, 
				char tape2WriteSymbol, char tape2Direction) {
        this.nextState = nextState;
        this.tape1WriteSymbol = tape1WriteSymbol;
        this.tape1Direction = tape1Direction;
        this.tape2WriteSymbol = tape2WriteSymbol;
        this.tape2Direction = tape2Direction;
    }

    char getTape1Direction() {
        return tape1Direction;
    }

    char getTape2Direction() {
        return tape2Direction;
    }
}
