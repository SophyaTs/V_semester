public class sProcess {
  public int cputime;
  public int ioblocking;
  public int iotime;
  public int cpudone;
  public int ionext;
  public int ioblockedleft;
  public int numblocked;

  public sProcess (int cputime, int ioblocking, int iotime, int cpudone, int ionext, int ioblockedleft, int numblocked) {
    this.cputime = cputime;
    this.ioblocking = ioblocking;
    this.iotime = iotime;
    this.cpudone = cpudone;
    this.ionext = ionext;
    this.ioblockedleft = ioblockedleft;
    this.numblocked = numblocked;
  } 	
}
