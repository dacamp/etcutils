if EU.can_lockfile?
  require 'etcutils_test_helper'
  class EULockingTest < Test::Unit::TestCase
    require "#{$eu_user}/locking"

    def test_methods
      assert EU.respond_to?(:lock), "Should respond to lock"
      assert EU.respond_to?(:lckpwdf), "Should respond to lckpwdf"

      assert EU.respond_to?(:locked?), "Should respond to locked?"
      assert EU.respond_to?(:unlock), "Should respond to unlock"
      assert EU.respond_to?(:ulckpwdf), "Should respond to ulckpwdf"
    end
  end
end
