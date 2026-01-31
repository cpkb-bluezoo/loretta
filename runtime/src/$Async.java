import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * $Async - Async runtime support using virtual threads (Java 21+).
 * 
 * Provides Python asyncio-like functionality:
 * - run() to execute a coroutine
 * - await() to wait for an awaitable
 * - gather() to run multiple coroutines concurrently
 * - sleep() for async sleep
 * 
 * Virtual threads allow millions of concurrent tasks with minimal overhead.
 * When a virtual thread blocks (I/O, sleep, await), it yields its carrier
 * thread to other virtual threads automatically.
 */
public final class $Async {
    
    // Virtual thread executor - creates a new virtual thread per task
    private static final ExecutorService EXECUTOR = 
        Executors.newVirtualThreadPerTaskExecutor();
    
    // Private constructor - all methods are static
    private $Async() {}
    
    /**
     * Run a coroutine/callable and return a Future.
     * The coroutine executes on a virtual thread.
     * 
     * @param coroutine The async function to run (as $MH callable)
     * @return A $Future that will contain the result
     */
    public static $Future run($MH coroutine) {
        CompletableFuture<$O> future = new CompletableFuture<>();
        
        Thread.startVirtualThread(() -> {
            try {
                $O result = coroutine.__call__();
                future.complete(result);
            } catch ($X e) {
                future.completeExceptionally(e);
            } catch (Throwable t) {
                future.completeExceptionally(new $X("RuntimeError", t.getMessage()));
            }
        });
        
        return new $Future(future);
    }
    
    /**
     * Run a coroutine/callable with arguments and return a Future.
     * 
     * @param coroutine The async function to run
     * @param args Arguments to pass
     * @return A $Future that will contain the result
     */
    public static $Future run($MH coroutine, $O... args) {
        CompletableFuture<$O> future = new CompletableFuture<>();
        
        Thread.startVirtualThread(() -> {
            try {
                $O result = coroutine.__call__(args);
                future.complete(result);
            } catch ($X e) {
                future.completeExceptionally(e);
            } catch (Throwable t) {
                future.completeExceptionally(new $X("RuntimeError", t.getMessage()));
            }
        });
        
        return new $Future(future);
    }
    
    /**
     * Await an awaitable object.
     * - If it's a $Future, block until complete (yields virtual thread)
     * - If it's a callable ($MH), execute it and return result
     * - Otherwise, return the value as-is
     * 
     * @param awaitable The object to await
     * @return The result
     */
    public static $O await($O awaitable) {
        if (awaitable == null) {
            return $N.INSTANCE;
        }
        
        if (awaitable instanceof $Future) {
            // Block virtual thread until future completes
            return (($Future) awaitable).get();
        }
        
        if (awaitable instanceof $MH) {
            // It's a callable - execute it
            // This handles the case where an async function returns a callable
            return (($MH) awaitable).__call__();
        }
        
        // Already a concrete value
        return awaitable;
    }
    
    /**
     * Run multiple coroutines concurrently and wait for all to complete.
     * Similar to asyncio.gather().
     * 
     * @param coroutines List of callables to run concurrently
     * @return List of results in the same order
     */
    public static $L gather($L coroutines) {
        List<CompletableFuture<$O>> futures = new ArrayList<>();
        
        // Start all coroutines on virtual threads
        for ($O coro : coroutines.items) {
            if (coro instanceof $MH) {
                CompletableFuture<$O> f = CompletableFuture.supplyAsync(
                    () -> (($MH) coro).__call__(),
                    EXECUTOR
                );
                futures.add(f);
            } else if (coro instanceof $Future) {
                futures.add((($Future) coro).getCompletableFuture());
            } else {
                // Already a value, wrap in completed future
                futures.add(CompletableFuture.completedFuture(coro));
            }
        }
        
        // Wait for all to complete
        CompletableFuture.allOf(futures.toArray(new CompletableFuture[0])).join();
        
        // Collect results
        $L results = new $L();
        for (CompletableFuture<$O> f : futures) {
            try {
                results.append(f.get());
            } catch (Exception e) {
                if (e.getCause() instanceof $X) {
                    throw ($X) e.getCause();
                }
                throw new $X("RuntimeError", e.getMessage());
            }
        }
        return results;
    }
    
    /**
     * Run multiple coroutines and return when the first one completes.
     * Similar to asyncio.wait() with FIRST_COMPLETED.
     * 
     * @param coroutines List of callables
     * @return The result of the first completed coroutine
     */
    public static $O first($L coroutines) {
        List<CompletableFuture<$O>> futures = new ArrayList<>();
        
        for ($O coro : coroutines.items) {
            if (coro instanceof $MH) {
                CompletableFuture<$O> f = CompletableFuture.supplyAsync(
                    () -> (($MH) coro).__call__(),
                    EXECUTOR
                );
                futures.add(f);
            } else if (coro instanceof $Future) {
                futures.add((($Future) coro).getCompletableFuture());
            }
        }
        
        // Return first completed
        CompletableFuture<Object> anyOf = CompletableFuture.anyOf(
            futures.toArray(new CompletableFuture[0])
        );
        
        try {
            return ($O) anyOf.get();
        } catch (Exception e) {
            if (e.getCause() instanceof $X) {
                throw ($X) e.getCause();
            }
            throw new $X("RuntimeError", e.getMessage());
        }
    }
    
    /**
     * Async sleep - suspends the virtual thread for the specified duration.
     * Does NOT block the carrier thread.
     * 
     * @param seconds Duration to sleep
     */
    public static void sleep(double seconds) {
        try {
            long millis = (long) (seconds * 1000);
            Thread.sleep(millis);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new $X("InterruptedError", "Sleep interrupted");
        }
    }
    
    /**
     * Async sleep returning a Future.
     * 
     * @param seconds Duration to sleep
     * @return Future that completes after the sleep
     */
    public static $Future sleepAsync(double seconds) {
        CompletableFuture<$O> future = new CompletableFuture<>();
        
        Thread.startVirtualThread(() -> {
            try {
                long millis = (long) (seconds * 1000);
                Thread.sleep(millis);
                future.complete($N.INSTANCE);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                future.completeExceptionally(new $X("InterruptedError", "Sleep interrupted"));
            }
        });
        
        return new $Future(future);
    }
    
    /**
     * Create a completed Future with a value.
     */
    public static $Future completed($O value) {
        return $Future.completed(value);
    }
    
    /**
     * Create an incomplete Future that can be completed later.
     */
    public static $Future createFuture() {
        return new $Future();
    }
    
    /**
     * Check if we're running on a virtual thread.
     */
    public static boolean isVirtualThread() {
        return Thread.currentThread().isVirtual();
    }
    
    /**
     * Get the current thread name (useful for debugging).
     */
    public static $S currentThread() {
        Thread t = Thread.currentThread();
        String name = t.getName();
        boolean virtual = t.isVirtual();
        return $S.of(name + (virtual ? " (virtual)" : " (platform)"));
    }
}
