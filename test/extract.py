import sys

# Check if a filename argument was provided
if len(sys.argv) != 2:
    print("Usage: python script.py <filename>")
    sys.exit(1)

# Get the filename from the command line argument
filename = sys.argv[1]

try:
    with open(filename, "r") as file:
        prev_time = None
        for line in file:
            # Check if the line contains "[CANLOG]"
            if "[CANLOG]" not in line:
                continue  # Skip lines without "[CANLOG]"

            # Find the position of "[CANLOG]"
            canlog_pos = line.find("[CANLOG]")
            
            # Extract the part of the line after "[CANLOG]" and strip leading/trailing whitespace
            canlog_data = line[canlog_pos + len("[CANLOG]") :].strip()

            # Split the extracted data into tokens using commas as separators
            tokens = canlog_data.split(",")

            # Check if there are enough tokens to process
            if len(tokens) >= 1:
                timestamp_str = tokens[0].strip()  # The first token is the timestamp

                try:
                    timestamp = int(timestamp_str)  # Convert the timestamp to an integer
                except ValueError:
                    # Handle lines with invalid timestamps (non-numeric)
                    print(f"Invalid timestamp on line: {line.strip()}")
                    continue  # Continue to the next line

                # Calculate the time difference between this line and the previous
                time_difference = timestamp - prev_time if prev_time is not None else 0

                # Update the previous timestamp with the current one
                prev_time = timestamp

                # Insert the time difference as a new token between token 0 and token 1
                tokens.insert(1, str(time_difference))

                # Reconstruct the modified line with the updated tokens
                modified_line = ",".join(tokens)
                print(modified_line)
except FileNotFoundError:
    print(f"File '{filename}' not found.")
except Exception as e:
    print(f"An error occurred: {str(e)}")
