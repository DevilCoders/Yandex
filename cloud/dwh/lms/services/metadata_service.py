class MetadataService:
    @staticmethod
    def parse_metadata(file_path: str):
        """
        Load metadata from a yml file
        :param file_path: str
        :return: YAMLObject
        """
        from yaml import load
        try:
            # from yaml import CDumper as Dumper, CSafeLoader as SafeLoader
            from yaml import CLoader as Loader
        except ImportError:
            from yaml import Loader
        with open(file_path, 'r') as f:
            return load(f, Loader=Loader)
