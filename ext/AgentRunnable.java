public final class AgentRunnable implements Runnable {
    private final long seq;

    public AgentRunnable(long seq) {
        this.seq = seq;
    }

    private static native void runNative(long seq);

    @Override
    public void run() {
        runNative(seq);
    }
}
