from helpers.kafkahelpers import create_producer, send_writer_command
from time import sleep
import h5py

def test_data_reaches_file(docker_compose):
    producer = create_producer()
    sleep(10)

    # Start file writing
    send_writer_command("commands/example-json-command.json", producer)
    producer.flush()
    # Give it some time to accumulate data
    sleep(10)
    # Stop file writing
    send_writer_command("commands/stop-command.json", producer)
    sleep(5)
    send_writer_command("commands/writer-exit.json", producer)
    producer.flush()

    # Allow time for the file writing to complete
    sleep(5)

    file = h5py.File("output-files/output_file.nxs", 'r')

    assert file is not None
    assert file.swmr_mode == False
    assert file["entry/start_time"].value == '2016-04-12T02:58:52'
    assert file["entry/end_time"].value == '2016-04-12T03:29:11'
    assert file["entry/duration"].value == 1817.0
    assert file["entry/features"][0] == 10138143369737381149
    assert file["entry/features"][1] == 17055242037422131565
    assert file["entry/title"][...] == 'Blend 1.9_SANS'
    assert file["entry/user_1/affiliation"].value == 'ISIS, STFC'
