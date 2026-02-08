# TODO

This document tracks planned improvements and known issues for the project.

## High Priority

### Buffer Management
- [ ] **Parametrize per-client buffer sizes**
  - Make minimum and maximum buffer sizes configurable
  - Add configuration options (environment variables, config file, or constructor parameters)
  - Document default values and recommended ranges

- [ ] **Implement dynamic buffer resizing**
  - Add logic to detect buffer overfill patterns
  - Add logic to detect buffer underfill patterns
  - Automatically grow buffers when consistently overfilled
  - Automatically shrink buffers when consistently underfilled
  - Add hysteresis to prevent oscillation
  - Define thresholds for triggering resize operations

### Parsing
- [ ] **Implement chunked parsing modes**
  - Mode 1: Callback invoked per chunk as data arrives
  - Mode 2: Assemble all chunks first, then single callback
  - Make parsing mode configurable per client or globally
  - Ensure memory efficiency for both modes

- [ ] **Fix header parsing with commas**
  - Issue: Parser may crash when comparing headers containing comma values
  - Root cause: Comparing against entire header value instead of parsing properly
  - Implement proper header value tokenization
  - Add test cases for headers with commas (e.g., `Accept: text/html, application/json`)

## Testing
- [ ] **Message sending tests**
  - Unit tests for various message sizes
  - Test boundary conditions (empty, max size, oversized)
  - Test concurrent sending from multiple clients
  - Verify message integrity and ordering

- [ ] **Client termination tests**
  - Test graceful client disconnection
  - Test forceful client termination
  - Verify resource cleanup (buffers, handles, connections)
  - Test termination under load
  - Ensure no memory leaks on termination

## Error Handling
- [ ] **IOCP error management**
  - Identify all possible IOCP error codes
  - Implement graceful handling for each error type
  - Add logging for IOCP errors with context
  - Ensure server stability when IOCP operations fail
  - Document error recovery strategies

## Future Enhancements
- [ ] Add metrics/statistics collection for buffer usage patterns
- [ ] Consider implementing buffer pooling for better memory management
- [ ] Add documentation for all public APIs
- [ ] Performance benchmarking suite

---

## Contributing
When working on items from this TODO list:
1. Create an issue referencing the TODO item
2. Create a feature branch
3. Include tests for new functionality
4. Update documentation as needed
5. Reference the TODO item in your pull request

## Notes
- Priority may shift based on user feedback and production issues
- Mark items as complete with `[x]` when merged to main branch