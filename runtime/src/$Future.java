import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * $Future - Awaitable future for async operations.
 * 
 * Wraps a CompletableFuture to provide Python-like awaitable semantics.
 * When awaited, blocks the current virtual thread (not the carrier thread)
 * until the result is available.
 */
public class $Future extends $O {
    
    private final CompletableFuture<$O> future;
    private $O result;
    private $X exception;
    private boolean done;
    
    /**
     * Create a future wrapping a CompletableFuture.
     */
    public $Future(CompletableFuture<$O> future) {
        this.future = future;
        this.done = false;
    }
    
    /**
     * Create an already-completed future with a result.
     */
    public $Future($O result) {
        this.future = CompletableFuture.completedFuture(result);
        this.result = result;
        this.done = true;
    }
    
    /**
     * Create a new incomplete future.
     */
    public $Future() {
        this.future = new CompletableFuture<>();
        this.done = false;
    }
    
    /**
     * Get the result, blocking if necessary.
     * This blocks the virtual thread, not the carrier thread.
     */
    public $O get() {
        if (done) {
            if (exception != null) {
                throw exception;
            }
            return result;
        }
        
        try {
            result = future.get();
            done = true;
            return result;
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new $X("InterruptedError", e.getMessage());
        } catch (ExecutionException e) {
            Throwable cause = e.getCause();
            if (cause instanceof $X) {
                exception = ($X) cause;
                throw exception;
            }
            exception = new $X("RuntimeError", cause.getMessage());
            throw exception;
        }
    }
    
    /**
     * Get the result with a timeout.
     */
    public $O get(double timeoutSeconds) {
        if (done) {
            if (exception != null) {
                throw exception;
            }
            return result;
        }
        
        try {
            long millis = (long) (timeoutSeconds * 1000);
            result = future.get(millis, TimeUnit.MILLISECONDS);
            done = true;
            return result;
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new $X("InterruptedError", e.getMessage());
        } catch (TimeoutException e) {
            throw new $X("TimeoutError", "Future timed out");
        } catch (ExecutionException e) {
            Throwable cause = e.getCause();
            if (cause instanceof $X) {
                exception = ($X) cause;
                throw exception;
            }
            exception = new $X("RuntimeError", cause.getMessage());
            throw exception;
        }
    }
    
    /**
     * Complete the future with a result.
     */
    public void setResult($O value) {
        future.complete(value);
        this.result = value;
        this.done = true;
    }
    
    /**
     * Complete the future with an exception.
     */
    public void setException($X exc) {
        future.completeExceptionally(exc);
        this.exception = exc;
        this.done = true;
    }
    
    /**
     * Check if the future is done.
     */
    public boolean isDone() {
        return done || future.isDone();
    }
    
    /**
     * Cancel the future.
     */
    public boolean cancel() {
        return future.cancel(true);
    }
    
    /**
     * Check if cancelled.
     */
    public boolean isCancelled() {
        return future.isCancelled();
    }
    
    /**
     * Get the underlying CompletableFuture.
     */
    public CompletableFuture<$O> getCompletableFuture() {
        return future;
    }
    
    @Override
    public boolean __bool__() {
        return true;
    }
    
    @Override
    public $S __repr__() {
        if (isDone()) {
            if (exception != null) {
                return $S.of("<Future exception=" + exception.type + ">");
            }
            return $S.of("<Future result=" + result + ">");
        }
        return $S.of("<Future pending>");
    }
    
    @Override
    public $S __str__() {
        return __repr__();
    }
    
    /**
     * Factory for a completed future.
     */
    public static $Future completed($O result) {
        return new $Future(result);
    }
    
    /**
     * Factory for a failed future.
     */
    public static $Future failed($X exception) {
        $Future f = new $Future();
        f.setException(exception);
        return f;
    }
}
