// Run() is called from Scheduling.main() and is where
// the scheduling algorithm written by the user resides.
// User modification should occur within the Run() function.

import java.util.Vector;
import java.util.Queue;
import java.util.List;
import java.util.ArrayList;
import java.util.LinkedList;
import java.io.*;

public class SchedulingAlgorithm {

	public static void progress(sProcess process, int timeUsed) {
		process.cpudone+=timeUsed;
	}
	
	public static void progressIO(sProcess process, int timeUsed) {
		process.cpudone+=timeUsed;
		process.ionext+=process.ioblocking;
		process.ioblockedleft = process.iotime;
		++process.numblocked;  
	}
	
	
	
  public static Results Run(int runtime, Vector<sProcess> processVector, Results result) {
    
    int comptime = 0;
    int currentProcess = 0;
    sProcess process = null;
    Queue<Integer> queue = new LinkedList<Integer>();
    List<Integer> blockedIO = new ArrayList<Integer>();
    final int quantum = 30;
    int actuallyUsed = 0;
    boolean io = false;
    int size = processVector.size();
    String resultsFile = "Summary-Processes";

    result.schedulingType = "Interactive (Preemptive)";
    result.schedulingName = "Round-Robin"; 
    try {
      PrintStream out = new PrintStream(new FileOutputStream(resultsFile));
      
      for(int i=0; i < size;++i) {
    	  process = (sProcess) processVector.elementAt(i);
    	  process.ionext = process.ioblocking;
    	  queue.add(i);
      }
      
      while (comptime < runtime) {
    	  if(queue.isEmpty() && blockedIO.isEmpty()) //all processes have been completed
    		  break;
    	  
    	  io = false;
    	  if(!queue.isEmpty()) {    		  
	    	  currentProcess = queue.poll();    	  
	    	  
	    	  process = (sProcess) processVector.elementAt(currentProcess);
	    	  out.println("Process: " + currentProcess + " gets quantum... (" + process.cputime + " " + process.ioblocking + " " + process.cpudone + ")");
	    	  
	    	  if(process.cputime - process.cpudone <= quantum) { // process may be completed in this quantum
	    		  if(process.ionext <= process.cputime) { // process gets IO blocked before completion
	    			  actuallyUsed = process.ionext - process.cpudone;
	    			  progressIO(process, actuallyUsed);
	    			  io = true;
	    			  out.println("Process: " + currentProcess + " I/O blocked... (" + process.cputime + " " + process.ioblocking + " " + process.cpudone + ")");
	    		  } 
	    		  else { // process actually finishes
	    			  actuallyUsed = process.cputime - process.cpudone;
	    			  progress(process, actuallyUsed);
	    			  out.println("Process: " + currentProcess + " completed... (" + process.cputime + " " + process.ioblocking + " " + process.cpudone + ")");
	    		  }
	    	  }
	    	  else { //process will not finish during this quantum
	    		  if(process.ionext <= process.cpudone + quantum) { // process gets IO blocked before quantum ends
	    			  actuallyUsed = process.ionext - process.cpudone;
	    			  progressIO(process, actuallyUsed);
	    			  io = true;
	    			  out.println("Process: " + currentProcess + " I/O blocked... (" + process.cputime + " " + process.ioblocking + " " + process.cpudone + ")");
	    		  }
	    		  else { // process gets blocked after quantum ends
	    			  actuallyUsed = quantum;
	    			  progress(process, actuallyUsed);
	    			  ++process.numblocked;
	    			  queue.add(currentProcess);
	    			  out.println("Process: " + currentProcess + " blocked... (" + process.cputime + " " + process.ioblocking + " " + process.cpudone + ")");
	    		  }
	    	  }	    	  	    	  
    	  } else { // there are only blocked processes
    		  process = (sProcess) processVector.elementAt(blockedIO.get(0));
    			actuallyUsed = process.ioblockedleft;		
    			for(int i: blockedIO)
    				process  = (sProcess) processVector.elementAt(i);
    				if(process .ioblockedleft < actuallyUsed)
    					actuallyUsed = process .ioblockedleft;
    		  out.println("All processes are blocked, waiting for at least one to become ready... "+actuallyUsed +" elapsed");
    	  }
	    		      
    	  if(!blockedIO.isEmpty())  {  // is there is any blocked processes, update their leftover blocked time
    		  for(int i = 0; i< blockedIO.size();++i) {
    			  process = (sProcess) processVector.elementAt(blockedIO.get(i));
    	  		  process.ioblockedleft = 0 < process.ioblockedleft-actuallyUsed ?
    	  				  process.ioblockedleft-actuallyUsed
    	  				  : 0;
    	  		  if (process.ioblockedleft == 0) {
    	  			  queue.add(blockedIO.get(i));
    	  			  blockedIO.remove(i);
    	  		  }
    	  	  }
    	  }
	    	 
	      
	      if(io == true)
	    		  blockedIO.add(currentProcess);
	      
	      comptime+=actuallyUsed;
      }
      out.close();
    } catch (IOException e) { /* Handle exceptions */ }
    result.compuTime = comptime;
    return result;
  }
}
