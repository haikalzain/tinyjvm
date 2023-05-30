class Exceptions {
    public int dangerousAdd(int x) {
        if(x == 0) {
            throw new RuntimeException("Cannot add");
        }
        return x + 1;
    }

    public void throwSomeException() throws Exception {
        throw new Exception("Something bad has happened");
    }


    public int countExceptions() {
        int caught = 0;

        try {
            throwSomeException();
        } catch(Exception e) {
            caught++;
        }

        try {
            dangerousAdd(0);
        } catch(RuntimeException e){
            caught++;
        }

        try {
            dangerousAdd(1);
        } catch(RuntimeException e){
            caught++;
        }
        return caught;
    }
}