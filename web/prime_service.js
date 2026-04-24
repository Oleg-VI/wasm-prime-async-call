/**
 * Thin JS wrapper around the WASM PrimeGenerator.
 * Public API uses camelCase; internally delegates to PascalCase C++ exports.
 */
export class PrimeService {
    /** @type {object|null} */
    #module = null;
    /** @type {object|null} */
    #generator = null;

    /**
     * Loads and initialises the WASM module. Must be awaited before any
     * other method is called.
     * @param {string} [wasmModulePath]
     */
    async initialize(wasmModulePath = './prime_module.js') {
        const { default: factory } = await import(wasmModulePath);
        this.#module    = await factory();
        this.#generator = new this.#module.PrimeGenerator();
    }

    /**
     * Starts async computation of all primes strictly below `limit`.
     *
     * @param {number}   limit       - Upper bound (exclusive).
     * @param {function} onProgress  - Called with percent (0–100) during computation.
     * @param {function} onComplete  - Called with number[] when done.
     * @returns {boolean} false if a computation is already running.
     */
    startComputation(limit, onProgress, onComplete) {
        this.#assertReady();
        return this.#generator.StartComputation(limit, onProgress, onComplete);
    }

    /** Cancels any in-progress computation. */
    cancel() {
        this.#assertReady();
        this.#generator.Cancel();
    }

    /** @returns {boolean} */
    get isRunning() {
        return this.#generator?.IsRunning() ?? false;
    }

    /** Frees the WASM-side object. Call when done. */
    destroy() {
        this.#generator?.delete();
        this.#generator = null;
    }

    #assertReady() {
        if (!this.#generator) throw new Error('PrimeService not initialised — call initialize() first.');
    }
}